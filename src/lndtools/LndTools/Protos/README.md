Commands to generate protobuf files for grpc:

Obviously `cpp` plugin `protoc` and has to be in PATH
- protoc -I/home/durkmurder/Work/Reps/xsn-wallet/depends/grpc/grpc/third_party/protobuf/src -I$PWD --grpc_out=$PWD --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` *.proto
- protoc -I/home/durkmurder/Work/Reps/xsn-wallet/depends/grpc/grpc/third_party/protobuf/src -I$PWD --cpp_out=$PWD *.proto


