#ifndef MARU_PERSPECTIVE_SERVICE_H
#define MARU_PERSPECTIVE_SERVICE_H

#include <utils/RefBase.h>

#include <perspective/IPerspectiveService.h>
#include <lxc/lxccontainer.h>

namespace android {

class PerspectiveService : public BnPerspectiveService {
public:
    static const char *getServiceName() {
        return "PerspectiveService";
    }

    PerspectiveService();

    int run();

private:
    // we are reference counted
    ~PerspectiveService();

    // IPerspectiveService interface
    // ------------------------------------------------------------

    virtual bool start();
    virtual bool stop();
    virtual bool isRunning();

    // ------------------------------------------------------------

    bool startContainer(struct lxc_container *c);

    bool isUeventFieldMatch(const char *s, const char *match, const int s_len);
    void ueventPoll();

    struct lxc_container *mContainer;

};

}; // namespace android

#endif // MARU_PERSPECTIVE_SERVICE_H
