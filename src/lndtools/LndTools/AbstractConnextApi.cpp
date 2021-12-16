#include "AbstractConnextApi.hpp"

//==============================================================================

AbstractConnextApi::AbstractConnextApi(QObject *parent) : QObject(parent)
{

}

//==============================================================================

ConnextApiException::ConnextApiException(QVariantMap error)
    : std::runtime_error(
          error.value("name", QString{ "Unspecified connext error" }).toString().toStdString())
    , code(error.value("code", QString{ "Unspecified connext error code" }).toString())
    , name(error.value("code", QString{ "Unspecified connext error name" }).toString())
    , msg(error.value("msg", QString{ "Unspecified connext error message" }).toString())
    , validationError(error.value("context", QString{ "Unspecified connext validation error" }).toMap().value("validationError").toString())
{
}

//==============================================================================
