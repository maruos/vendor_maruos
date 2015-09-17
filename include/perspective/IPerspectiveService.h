#ifndef MARU_IPERSPECTIVE_SERVICE_H
#define MARU_IPERSPECTIVE_SERVICE_H

#include <utils/Errors.h>
#include <binder/IInterface.h>

namespace android {

/**
 * Define the Binder API for perspectived.
 */
class IPerspectiveService : public IInterface {
public:
    DECLARE_META_INTERFACE(PerspectiveService);

    virtual bool start() = 0;

    virtual bool stop() = 0;

    virtual bool isRunning() = 0;
};

/**
 * This will be implemented by the actual PerspectiveService.
 */
class BnPerspectiveService : public BnInterface<IPerspectiveService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

}; // namespace android

#endif // MARU_IPERSPECTIVE_SERVICE_H
