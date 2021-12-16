#ifndef GENERICPROTODATABASE_HPP
#define GENERICPROTODATABASE_HPP

#include <dbwrapper.h>

#include <QObject>
#include <boost/optional.hpp>
#include <memory>
#include <unordered_map>

namespace Utils {

//==============================================================================

class LevelDBSharedDatabase : public bitcoin::CDBWrapper {
public:
    LevelDBSharedDatabase(const fs::path& path, size_t nCacheSize)
        : bitcoin::CDBWrapper(path, nCacheSize)
        , _path(path.string())
    {
    }

    void registerIndex(std::string id, std::string index)
    {
        Q_ASSERT_X(_indexes.count(id) == 0, __FUNCTION__, "Not unique index");
        _indexes.emplace(id, index);
    }

    std::string indexById(std::string id) const { return _indexes.at(id); }

    std::string path() const { return _path; }

private:
    std::string _path;
    std::unordered_map<std::string, std::string> _indexes;
};

//==============================================================================

template <typename ValueType> class GenericProtoDatabase {
public:
    using Cache = std::unordered_map<uint64_t, ValueType>;
    explicit GenericProtoDatabase(
        std::shared_ptr<LevelDBSharedDatabase> provider, std::string index)
        : _provider(provider)
    {
        _provider->registerIndex(index, index);
        _index = index;
    }

    void load();
    bool save(ValueType& entry);
    bool update(std::vector<ValueType> entries);
    void erase(std::vector<uint64_t> id);
    bool exists(uint64_t id) const { return _values.count(id) > 0; }
    boost::optional<ValueType> get(uint64_t id) const
    {
        return exists(id) ? boost::make_optional(_values.at(id)) : boost::none;
    }
    const Cache& values() const { return _values; }

private:
    Cache _values;
    std::shared_ptr<LevelDBSharedDatabase> _provider;
    std::string _index;
    uint64_t _lastGeneratedId{ 0 };
};

//==============================================================================

template <typename ValueType> void GenericProtoDatabase<ValueType>::load()
{
    using namespace bitcoin;
    std::unique_ptr<CDBIterator> pcursor(_provider->NewIterator());
    std::pair<std::string, uint64_t> key;
    pcursor->Seek(_index);

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        if (pcursor->GetKey(key) && key.first == _index) {
            ValueType entry;
            std::string data;
            if (pcursor->GetValue(data)) {
                entry.ParseFromString(data);
                Q_ASSERT(entry.id() > 0);
                Q_ASSERT(entry.id() == key.second);
                _values.emplace(entry.id(), entry);
                _lastGeneratedId = std::max(_lastGeneratedId, entry.id());
                pcursor->Next();
            } else {
                throw std::runtime_error("Failed to value from repository, index: " + _index);
            }
        } else {
            break;
        }
    }
}

//==============================================================================

template <typename ValueType> bool GenericProtoDatabase<ValueType>::save(ValueType& entry)
{
    if (entry.id() == 0) {
        do {
            entry.set_id(++_lastGeneratedId);
        } while (exists(entry.id()) && entry.id() > 0);
    }

    Q_ASSERT(entry.id() > 0);

    _values.emplace(entry.id(), entry);
    return _provider->Write(std::make_pair(_index, entry.id()), entry.SerializeAsString());
}

//==============================================================================

template <typename ValueType>
bool GenericProtoDatabase<ValueType>::update(std::vector<ValueType> entries)
{
    bitcoin::CDBBatch batch(*_provider);
    bool result = true;
    for (auto&& entry : entries) {
        if (exists(entry.id())) {
            batch.Write(std::make_pair(_index, entry.id()), entry.SerializeAsString());
            _values[entry.id()] = entry;
        }
    }

    return _provider->WriteBatch(batch, true) && result;
}

//==============================================================================

template <typename ValueType> void GenericProtoDatabase<ValueType>::erase(std::vector<uint64_t> ids)
{
    bitcoin::CDBBatch batch(*_provider);
    for (auto&& id : ids) {
        auto it = _values.find(id);
        if (it != std::end(_values)) {
            batch.Erase(std::make_pair(_index, id));
            _values.erase(it);
        }
    }

    _provider->WriteBatch(batch, true);
}

//==============================================================================
}

#endif // GENERICPROTODATABASE_HPP
