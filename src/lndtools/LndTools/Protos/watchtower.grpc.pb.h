// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: watchtower.proto
#ifndef GRPC_watchtower_2eproto__INCLUDED
#define GRPC_watchtower_2eproto__INCLUDED

#include "watchtower.pb.h"

#include <functional>
#include <grpc/impl/codegen/port_platform.h>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace watchtowerrpc {

class Watchtower final {
 public:
  static constexpr char const* service_full_name() {
    return "watchtowerrpc.Watchtower";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    // * lncli: tower info
    // GetInfo returns general information concerning the companion watchtower
    // including its public key and URIs where the server is currently
    // listening for clients.
    virtual ::grpc::Status GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::watchtowerrpc::GetInfoResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>> AsyncGetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>>(AsyncGetInfoRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>> PrepareAsyncGetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>>(PrepareAsyncGetInfoRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      // * lncli: tower info
      // GetInfo returns general information concerning the companion watchtower
      // including its public key and URIs where the server is currently
      // listening for clients.
      virtual void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, std::function<void(::grpc::Status)>) = 0;
      virtual void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, std::function<void(::grpc::Status)>) = 0;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      virtual void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      #else
      virtual void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      #endif
    };
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    typedef class experimental_async_interface async_interface;
    #endif
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    async_interface* async() { return experimental_async(); }
    #endif
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>* AsyncGetInfoRaw(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::watchtowerrpc::GetInfoResponse>* PrepareAsyncGetInfoRaw(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::watchtowerrpc::GetInfoResponse* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>> AsyncGetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>>(AsyncGetInfoRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>> PrepareAsyncGetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>>(PrepareAsyncGetInfoRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, std::function<void(::grpc::Status)>) override;
      void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, std::function<void(::grpc::Status)>) override;
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void GetInfo(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
      #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
      #else
      void GetInfo(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::watchtowerrpc::GetInfoResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      #endif
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>* AsyncGetInfoRaw(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::watchtowerrpc::GetInfoResponse>* PrepareAsyncGetInfoRaw(::grpc::ClientContext* context, const ::watchtowerrpc::GetInfoRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_GetInfo_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    // * lncli: tower info
    // GetInfo returns general information concerning the companion watchtower
    // including its public key and URIs where the server is currently
    // listening for clients.
    virtual ::grpc::Status GetInfo(::grpc::ServerContext* context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_GetInfo() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestGetInfo(::grpc::ServerContext* context, ::watchtowerrpc::GetInfoRequest* request, ::grpc::ServerAsyncResponseWriter< ::watchtowerrpc::GetInfoResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_GetInfo<Service > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_GetInfo() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::watchtowerrpc::GetInfoRequest, ::watchtowerrpc::GetInfoResponse>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::watchtowerrpc::GetInfoRequest* request, ::watchtowerrpc::GetInfoResponse* response) { return this->GetInfo(context, request, response); }));}
    void SetMessageAllocatorFor_GetInfo(
        ::grpc::experimental::MessageAllocator< ::watchtowerrpc::GetInfoRequest, ::watchtowerrpc::GetInfoResponse>* allocator) {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
    #else
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::experimental().GetHandler(0);
    #endif
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::watchtowerrpc::GetInfoRequest, ::watchtowerrpc::GetInfoResponse>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* GetInfo(
      ::grpc::CallbackServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* GetInfo(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/)
    #endif
      { return nullptr; }
  };
  #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
  typedef ExperimentalWithCallbackMethod_GetInfo<Service > CallbackService;
  #endif

  typedef ExperimentalWithCallbackMethod_GetInfo<Service > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_GetInfo() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_GetInfo() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestGetInfo(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_GetInfo() {
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
      ::grpc::Service::
    #else
      ::grpc::Service::experimental().
    #endif
        MarkMethodRawCallback(0,
          new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
                   ::grpc::CallbackServerContext*
    #else
                   ::grpc::experimental::CallbackServerContext*
    #endif
                     context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->GetInfo(context, request, response); }));
    }
    ~ExperimentalWithRawCallbackMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    #ifdef GRPC_CALLBACK_API_NONEXPERIMENTAL
    virtual ::grpc::ServerUnaryReactor* GetInfo(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #else
    virtual ::grpc::experimental::ServerUnaryReactor* GetInfo(
      ::grpc::experimental::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)
    #endif
      { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_GetInfo : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_GetInfo() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::watchtowerrpc::GetInfoRequest, ::watchtowerrpc::GetInfoResponse>(std::bind(&WithStreamedUnaryMethod_GetInfo<BaseClass>::StreamedGetInfo, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_GetInfo() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status GetInfo(::grpc::ServerContext* /*context*/, const ::watchtowerrpc::GetInfoRequest* /*request*/, ::watchtowerrpc::GetInfoResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedGetInfo(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::watchtowerrpc::GetInfoRequest,::watchtowerrpc::GetInfoResponse>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_GetInfo<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_GetInfo<Service > StreamedService;
};

}  // namespace watchtowerrpc


#endif  // GRPC_watchtower_2eproto__INCLUDED
