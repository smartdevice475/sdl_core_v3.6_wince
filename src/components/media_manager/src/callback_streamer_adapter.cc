/**
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef MODIFY_FUNCTION_SIGN
#include <global_first.h>
#endif
#include <errno.h>
#include <unistd.h>
#include "utils/logger.h"
#include "config_profile/profile.h"
#include "media_manager/callback_streamer_adapter.h"
#ifdef MODIFY_FUNCTION_SIGN
#ifdef ENABLE_LOG
#include "utils/file_system.h"
#endif
#endif

namespace media_manager {

CREATE_LOGGERPTR_GLOBAL(logger, "CallbackStreamerAdapter")

CallbackStreamerAdapter::CallbackStreamerAdapter()
  : is_ready_(false),
    messages_(),
    thread_(NULL){
  LOG4CXX_INFO(logger, "CallbackStreamerAdapter::CallbackStreamerAdapter");
}

CallbackStreamerAdapter::~CallbackStreamerAdapter() {
  LOG4CXX_INFO(logger, "CallbackStreamerAdapter::~CallbackStreamerAdapter");

  if ((0 != current_application_ ) && (is_ready_)) {
    StopActivity(current_application_);
  }

  thread_->stop();
  delete thread_;
}

void CallbackStreamerAdapter::SendData(
  int32_t application_key,
  const protocol_handler::RawMessagePtr& message) {
  LOG4CXX_INFO(logger, "CallbackStreamerAdapter::SendData");

  if (application_key != current_application_) {
    LOG4CXX_WARN(logger, "Wrong application " << application_key);
    return;
  }

  if (is_ready_) {
    messages_.push(message);
  }
}

void CallbackStreamerAdapter::StartActivity(int32_t application_key) {
  LOG4CXX_INFO(logger, "CallbackStreamerAdapter::StartActivity");

  if (application_key == current_application_) {
    LOG4CXX_WARN(logger, "Already started activity for " << application_key);
    return;
  }

  current_application_ = application_key;
  is_ready_ = true;

  for (std::set<MediaListenerPtr>::iterator it = media_listeners_.begin();
       media_listeners_.end() != it;
       ++it) {
    (*it)->OnActivityStarted(application_key);
  }

  LOG4CXX_INFO(logger, "Callback start for writing ");
}

void CallbackStreamerAdapter::StopActivity(int32_t application_key) {
  LOG4CXX_INFO(logger, "CallbackStreamerAdapter::StopActivity");

  if (application_key != current_application_) {
    LOG4CXX_WARN(logger, "Not performing activity for " << application_key);
    return;
  }

  is_ready_ = false;
  current_application_ = 0;

  for (std::set<MediaListenerPtr>::iterator it = media_listeners_.begin();
       media_listeners_.end() != it;
       ++it) {
    (*it)->OnActivityEnded(application_key);
  }
}

bool CallbackStreamerAdapter::is_app_performing_activity(
  int32_t application_key) {
  return (application_key == current_application_);
}

void CallbackStreamerAdapter::Init() {
  if (!thread_) {
    LOG4CXX_INFO(logger, "Create and start sending thread");
    thread_ = new threads::Thread("CallbackStreamerAdapter", new Streamer(this));
    const size_t kStackSize = 16384;
    thread_->startWithOptions(threads::ThreadOptions(kStackSize));
  }
}

CallbackStreamerAdapter::Streamer::Streamer(
  CallbackStreamerAdapter* server)
  : server_(server),
    stop_flag_(false) {
}

CallbackStreamerAdapter::Streamer::~Streamer() {
  server_ = NULL;
}

void CallbackStreamerAdapter::Streamer::threadMain() {
  LOG4CXX_INFO(logger, "Streamer::threadMain");

  open();

  while (!stop_flag_) {
    while (!server_->messages_.empty()) {
      protocol_handler::RawMessagePtr msg = server_->messages_.pop();
      if (!msg) {
        LOG4CXX_ERROR(logger, "Null pointer message");
        continue;
      }


			ssize_t ret = 0;
#ifdef BUILD_TARGET_LIB
			//PRINTMSG(1, (L"\nvideostream ready to write data(size is %d):", msg.get()->data_size()));
			s_mediaVideoStreamSendCallback((const char *)msg.get()->data(), msg.get()->data_size());
			ret = msg.get()->data_size();
#endif
			//PRINTMSG(1, (L"\nvideostream write data(size is %d):", msg.get()->data_size()));
			//for(int i = 0; i < msg.get()->data_size(); i++){
			//	PRINTMSG(1, (L"%02x ", msg.get()->data()[i]));
			//}
			//PRINTMSG(1, (L"\n"));

      if (0 == ret) {
        LOG4CXX_ERROR(logger, "Callback failed writing data to function "
                      );

        std::set<MediaListenerPtr>::iterator it =
            server_->media_listeners_.begin();
        for (;server_->media_listeners_.end() != it; ++it) {
          (*it)->OnErrorReceived(server_->current_application_, -1);
        }
      } else if (ret != msg.get()->data_size()) {
        LOG4CXX_WARN(logger, "Couldn't write all the data to callback ");
      }

      static int32_t messsages_for_session = 0;
      ++messsages_for_session;

      LOG4CXX_INFO(logger, "Handling map streaming message. This is "
                   << messsages_for_session << " the message for "
                   << server_->current_application_);
      std::set<MediaListenerPtr>::iterator it =
          server_->media_listeners_.begin();
      for (; server_->media_listeners_.end() != it; ++it) {
        (*it)->OnDataReceived(server_->current_application_,
                              messsages_for_session);
      }
    }
    server_->messages_.wait();
  }
  close();
}

bool CallbackStreamerAdapter::Streamer::exitThreadMain() {
  LOG4CXX_INFO(logger, "Streamer::exitThreadMain");
  stop_flag_ = true;
  server_->messages_.Shutdown();
  return false;
}

void CallbackStreamerAdapter::Streamer::open() {
	// not implement
}

void CallbackStreamerAdapter::Streamer::close() {
	// not implement
}

}  // namespace media_manager
