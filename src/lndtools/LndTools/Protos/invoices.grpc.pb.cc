// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: invoices.proto

#include "invoices.pb.h"
#include "invoices.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace invoicesrpc {

static const char* Invoices_method_names[] = {
  "/invoicesrpc.Invoices/SubscribeSingleInvoice",
  "/invoicesrpc.Invoices/CancelInvoice",
  "/invoicesrpc.Invoices/AddHoldInvoice",
  "/invoicesrpc.Invoices/SettleInvoice",
};

std::unique_ptr< Invoices::Stub> Invoices::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< Invoices::Stub> stub(new Invoices::Stub(channel));
  return stub;
}

Invoices::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_SubscribeSingleInvoice_(Invoices_method_names[0], ::grpc::internal::RpcMethod::SERVER_STREAMING, channel)
  , rpcmethod_CancelInvoice_(Invoices_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_AddHoldInvoice_(Invoices_method_names[2], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_SettleInvoice_(Invoices_method_names[3], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::ClientReader< ::lnrpc::Invoice>* Invoices::Stub::SubscribeSingleInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::SubscribeSingleInvoiceRequest& request) {
  return ::grpc_impl::internal::ClientReaderFactory< ::lnrpc::Invoice>::Create(channel_.get(), rpcmethod_SubscribeSingleInvoice_, context, request);
}

void Invoices::Stub::experimental_async::SubscribeSingleInvoice(::grpc::ClientContext* context, ::invoicesrpc::SubscribeSingleInvoiceRequest* request, ::grpc::experimental::ClientReadReactor< ::lnrpc::Invoice>* reactor) {
  ::grpc_impl::internal::ClientCallbackReaderFactory< ::lnrpc::Invoice>::Create(stub_->channel_.get(), stub_->rpcmethod_SubscribeSingleInvoice_, context, request, reactor);
}

::grpc::ClientAsyncReader< ::lnrpc::Invoice>* Invoices::Stub::AsyncSubscribeSingleInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::SubscribeSingleInvoiceRequest& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc_impl::internal::ClientAsyncReaderFactory< ::lnrpc::Invoice>::Create(channel_.get(), cq, rpcmethod_SubscribeSingleInvoice_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::lnrpc::Invoice>* Invoices::Stub::PrepareAsyncSubscribeSingleInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::SubscribeSingleInvoiceRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncReaderFactory< ::lnrpc::Invoice>::Create(channel_.get(), cq, rpcmethod_SubscribeSingleInvoice_, context, request, false, nullptr);
}

::grpc::Status Invoices::Stub::CancelInvoice(::grpc::ClientContext* context, const ::invoicesrpc::CancelInvoiceMsg& request, ::invoicesrpc::CancelInvoiceResp* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_CancelInvoice_, context, request, response);
}

void Invoices::Stub::experimental_async::CancelInvoice(::grpc::ClientContext* context, const ::invoicesrpc::CancelInvoiceMsg* request, ::invoicesrpc::CancelInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_CancelInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::CancelInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::CancelInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_CancelInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::CancelInvoice(::grpc::ClientContext* context, const ::invoicesrpc::CancelInvoiceMsg* request, ::invoicesrpc::CancelInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_CancelInvoice_, context, request, response, reactor);
}

void Invoices::Stub::experimental_async::CancelInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::CancelInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_CancelInvoice_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::CancelInvoiceResp>* Invoices::Stub::AsyncCancelInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::CancelInvoiceMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::CancelInvoiceResp>::Create(channel_.get(), cq, rpcmethod_CancelInvoice_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::CancelInvoiceResp>* Invoices::Stub::PrepareAsyncCancelInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::CancelInvoiceMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::CancelInvoiceResp>::Create(channel_.get(), cq, rpcmethod_CancelInvoice_, context, request, false);
}

::grpc::Status Invoices::Stub::AddHoldInvoice(::grpc::ClientContext* context, const ::invoicesrpc::AddHoldInvoiceRequest& request, ::invoicesrpc::AddHoldInvoiceResp* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_AddHoldInvoice_, context, request, response);
}

void Invoices::Stub::experimental_async::AddHoldInvoice(::grpc::ClientContext* context, const ::invoicesrpc::AddHoldInvoiceRequest* request, ::invoicesrpc::AddHoldInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_AddHoldInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::AddHoldInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::AddHoldInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_AddHoldInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::AddHoldInvoice(::grpc::ClientContext* context, const ::invoicesrpc::AddHoldInvoiceRequest* request, ::invoicesrpc::AddHoldInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_AddHoldInvoice_, context, request, response, reactor);
}

void Invoices::Stub::experimental_async::AddHoldInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::AddHoldInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_AddHoldInvoice_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::AddHoldInvoiceResp>* Invoices::Stub::AsyncAddHoldInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::AddHoldInvoiceRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::AddHoldInvoiceResp>::Create(channel_.get(), cq, rpcmethod_AddHoldInvoice_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::AddHoldInvoiceResp>* Invoices::Stub::PrepareAsyncAddHoldInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::AddHoldInvoiceRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::AddHoldInvoiceResp>::Create(channel_.get(), cq, rpcmethod_AddHoldInvoice_, context, request, false);
}

::grpc::Status Invoices::Stub::SettleInvoice(::grpc::ClientContext* context, const ::invoicesrpc::SettleInvoiceMsg& request, ::invoicesrpc::SettleInvoiceResp* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_SettleInvoice_, context, request, response);
}

void Invoices::Stub::experimental_async::SettleInvoice(::grpc::ClientContext* context, const ::invoicesrpc::SettleInvoiceMsg* request, ::invoicesrpc::SettleInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_SettleInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::SettleInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::SettleInvoiceResp* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_SettleInvoice_, context, request, response, std::move(f));
}

void Invoices::Stub::experimental_async::SettleInvoice(::grpc::ClientContext* context, const ::invoicesrpc::SettleInvoiceMsg* request, ::invoicesrpc::SettleInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_SettleInvoice_, context, request, response, reactor);
}

void Invoices::Stub::experimental_async::SettleInvoice(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::invoicesrpc::SettleInvoiceResp* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_SettleInvoice_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::SettleInvoiceResp>* Invoices::Stub::AsyncSettleInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::SettleInvoiceMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::SettleInvoiceResp>::Create(channel_.get(), cq, rpcmethod_SettleInvoice_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::invoicesrpc::SettleInvoiceResp>* Invoices::Stub::PrepareAsyncSettleInvoiceRaw(::grpc::ClientContext* context, const ::invoicesrpc::SettleInvoiceMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::invoicesrpc::SettleInvoiceResp>::Create(channel_.get(), cq, rpcmethod_SettleInvoice_, context, request, false);
}

Invoices::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Invoices_method_names[0],
      ::grpc::internal::RpcMethod::SERVER_STREAMING,
      new ::grpc::internal::ServerStreamingHandler< Invoices::Service, ::invoicesrpc::SubscribeSingleInvoiceRequest, ::lnrpc::Invoice>(
          std::mem_fn(&Invoices::Service::SubscribeSingleInvoice), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Invoices_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Invoices::Service, ::invoicesrpc::CancelInvoiceMsg, ::invoicesrpc::CancelInvoiceResp>(
          std::mem_fn(&Invoices::Service::CancelInvoice), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Invoices_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Invoices::Service, ::invoicesrpc::AddHoldInvoiceRequest, ::invoicesrpc::AddHoldInvoiceResp>(
          std::mem_fn(&Invoices::Service::AddHoldInvoice), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Invoices_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Invoices::Service, ::invoicesrpc::SettleInvoiceMsg, ::invoicesrpc::SettleInvoiceResp>(
          std::mem_fn(&Invoices::Service::SettleInvoice), this)));
}

Invoices::Service::~Service() {
}

::grpc::Status Invoices::Service::SubscribeSingleInvoice(::grpc::ServerContext* context, const ::invoicesrpc::SubscribeSingleInvoiceRequest* request, ::grpc::ServerWriter< ::lnrpc::Invoice>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Invoices::Service::CancelInvoice(::grpc::ServerContext* context, const ::invoicesrpc::CancelInvoiceMsg* request, ::invoicesrpc::CancelInvoiceResp* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Invoices::Service::AddHoldInvoice(::grpc::ServerContext* context, const ::invoicesrpc::AddHoldInvoiceRequest* request, ::invoicesrpc::AddHoldInvoiceResp* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Invoices::Service::SettleInvoice(::grpc::ServerContext* context, const ::invoicesrpc::SettleInvoiceMsg* request, ::invoicesrpc::SettleInvoiceResp* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace invoicesrpc
