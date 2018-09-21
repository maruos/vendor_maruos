/*
 * Copyright 2015-2016 Preetam J. D'Souza
 * Copyright 2016 The Maru OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    virtual bool enableInput(bool enable) = 0;
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
