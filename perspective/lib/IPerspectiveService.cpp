#include <binder/Parcel.h>

#include <perspective/IPerspectiveService.h>

namespace android {

enum {
    START = IBinder::FIRST_CALL_TRANSACTION,
    STOP,
    IS_RUNNING
};

/**
 * Client-side transaction logic (aka Binder proxy) for PerspectiveService.
 */
class BpPerspectiveService : public BpInterface<IPerspectiveService> {
public:
    BpPerspectiveService(const sp<IBinder>& impl)
        : BpInterface<IPerspectiveService>(impl)
    {
    }

    virtual bool start() {
        Parcel data, reply;
        data.writeInterfaceToken(IPerspectiveService::getInterfaceDescriptor());
        remote()->transact(START, data, &reply);
        return reply.readInt32() != 0;
    }

    virtual bool stop() {
        Parcel data, reply;
        data.writeInterfaceToken(IPerspectiveService::getInterfaceDescriptor());
        remote()->transact(STOP, data, &reply);
        return reply.readInt32() != 0;
    }

    virtual bool isRunning() {
        Parcel data, reply;
        data.writeInterfaceToken(IPerspectiveService::getInterfaceDescriptor());
        remote()->transact(IS_RUNNING, data, &reply);
        return reply.readInt32() != 0;
    }

};

IMPLEMENT_META_INTERFACE(PerspectiveService, "maru.PerspectiveService");

// ----------------------------------------------------------------------------------

/**
 * Server-side transaction logic.
 */
status_t BnPerspectiveService::onTransact(
    uint32_t code, const Parcel& data, Parcel *reply, uint32_t flags) {
    switch(code) {
        case START: {
            CHECK_INTERFACE(IPerspectiveService, data, reply);
            reply->writeInt32(start() ? 1 : 0);
            return NO_ERROR;
        }
        case STOP: {
            CHECK_INTERFACE(IPerspectiveService, data, reply);
            reply->writeInt32(stop() ? 1 : 0);
            return NO_ERROR;
        }
        case IS_RUNNING: {
            CHECK_INTERFACE(IPerspectiveService, data, reply);
            reply->writeInt32(isRunning() ? 1 : 0);
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }

    return NO_ERROR;
}

}; // namespace android
