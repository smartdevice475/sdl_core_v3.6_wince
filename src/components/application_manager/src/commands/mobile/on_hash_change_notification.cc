
/*

 Copyright (c) 2013, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef MODIFY_FUNCTION_SIGN
#include <global_first.h>
#endif
#include "application_manager/commands/mobile/on_hash_change_notification.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "interfaces/MOBILE_API.h"
#include <string>
#ifdef OS_ANDROID
#include <sstream>
#elif defined(OS_WIN32)
#include <sstream>
#elif defined(OS_LINUX)
#include <sstream>
#endif

namespace application_manager {

namespace commands {

namespace mobile {

OnHashChangeNotification::OnHashChangeNotification(
    const MessageSharedPtr& message)
    : CommandNotificationImpl(message) {
}

OnHashChangeNotification::~OnHashChangeNotification() {
}

void OnHashChangeNotification::Run() {
  LOG4CXX_INFO(logger_, "OnHashChangeNotification::Run");

  (*message_)[strings::params][strings::message_type] =
      static_cast<int32_t>(application_manager::MessageType::kNotification);

  int32_t app_id;
  app_id = (*message_)[strings::params][strings::connection_key].asInt();
  ApplicationSharedPtr app = ApplicationManagerImpl::instance()->application(app_id);
  std::stringstream stream;
  stream << app->curHash();
  (*message_)[strings::msg_params][strings::hash_id] = stream.str();
  SendNotification();
}

}  //namespace mobile

}  // namespace commands

}  // namespace application_manager
