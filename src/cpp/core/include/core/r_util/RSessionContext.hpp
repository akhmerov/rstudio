/*
 * RSessionContext.hpp
 *
 * Copyright (C) 2009-15 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_R_UTIL_R_SESSION_CONTEXT_HPP
#define CORE_R_UTIL_R_SESSION_CONTEXT_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <boost/function.hpp>

#include <core/system/UserObfuscation.hpp>

#define kProjectNone   "none"
#define kUserIdLen      5
#define kProjectIdLen   8
#define kProjectNoneId "cfc78a31"
#define kWorkspacesId  "3c286bd3"

#ifdef _WIN32
typedef unsigned int uid_t;
inline uid_t getuid()
{
   return 0;
}
#endif

namespace rstudio {
namespace core {

class FilePath;

namespace r_util {

inline std::string obfuscatedUserId(uid_t uid)
{
   std::ostringstream ustr;
   ustr << std::setw(kUserIdLen) << std::setfill('0') << std::hex
        << OBFUSCATE_USER_ID(uid);
   return ustr.str();
}

inline uid_t deobfuscatedUserId(const std::string& userId)
{
   // shortcut if user ID is not known
   if (userId.empty())
      return 0;

   // attempt to parse user id
   long uid = strtol(userId.c_str(), NULL, 16);

   // collapse all error cases
   if (uid == LONG_MAX || uid == LONG_MIN)
      uid = 0;
   else if (uid != 0)
      uid = DEOBFUSCATE_USER_ID(uid);
   return uid;
}

class ProjectId
{
public:
   ProjectId()
   {}

   ProjectId(const std::string &id)
   {
      if (id.length() == kProjectIdLen)
      {
         id_ = id;
         userId_ = obfuscatedUserId(::getuid());
      }
      else if (id.length() == (kProjectIdLen + kUserIdLen))
      {
         userId_ = id.substr(0, kUserIdLen);
         id_ = id.substr(kUserIdLen);
      }
   }

   ProjectId(const std::string& id, const std::string& userId):
      id_(id), userId_(userId)
   {}

   ProjectId(const std::string& id, uid_t userId):
      id_(id)
   {
      userId_ = obfuscatedUserId(userId);
   }

   std::string asString() const
   {
      return userId_ + id_;
   }

   bool operator==(const ProjectId &other) const
   {
      return id_  == other.id_ && userId_ == other.userId_;
   }

   bool operator<(const ProjectId &other) const
   {
       return id_ < other.id_ ||
              (id_  == other.id_  && userId_  < other.userId_);
   }

   const std::string& id() const
   {
      return id_;
   }

   const std::string& userId() const
   {
      return userId_;
   }

private:
   std::string id_;
   std::string userId_;
};

typedef boost::function<std::string(const ProjectId&)> ProjectIdToFilePath;
typedef boost::function<ProjectId(const std::string&)> FilePathToProjectId;

class SessionScope
{
private:
   SessionScope(const ProjectId& project, const std::string& id)
      : project_(project), id_(id)
   {
   }

public:

   static SessionScope fromProject(
                           const std::string& project,
                           const std::string& id,
                           const FilePathToProjectId& filePathToProjectId);

   static std::string projectPathForScope(
                           const SessionScope& scope,
                           const ProjectIdToFilePath& projectIdToFilePath);

   static SessionScope fromProjectId(const ProjectId& project,
                                     const std::string& id);

   static SessionScope projectNone(const std::string& id);

   SessionScope()
   {
   }

   bool isProjectNone() const;

   bool isWorkspaces() const;

   const std::string project() const { return project_.asString(); }

   const std::string& id() const { return id_; }

   const ProjectId& projectId() const { return project_; }

   bool empty() const { return project_.id().empty(); }

   bool operator==(const SessionScope &other) const {
      return project_ == other.project_ && id_ == other.id_;
   }

   bool operator!=(const SessionScope &other) const {
      return !(*this == other);
   }

   bool operator<(const SessionScope &other) const {
       return project_ < other.project_ ||
              (project_ == other.project_ && id_ < other.id_);
   }

private:
   ProjectId project_;
   std::string id_;
};

bool validateSessionScope(const SessionScope& scope,
                          const core::FilePath& userHomePath,
                          const core::FilePath& userScratchPath,
                          core::r_util::ProjectIdToFilePath projectIdToFilePath,
                          std::string* pProjectFilePath);

std::string urlPathForSessionScope(const SessionScope& scope);

std::string createSessionUrl(const std::string& hostPageUrl,
                             const SessionScope& scope);

void parseSessionUrl(const std::string& url,
                     SessionScope* pScope,
                     std::string* pUrlPrefix,
                     std::string* pUrlWithoutPrefix);


struct SessionContext
{
   SessionContext()
   {
   }

   explicit SessionContext(const std::string& username,
                           const SessionScope& scope = SessionScope())
      : username(username), scope(scope)
   {
   }
   std::string username;
   SessionScope scope;

   bool empty() const { return username.empty(); }

   bool operator==(const SessionContext &other) const {
      return username == other.username && scope == other.scope;
   }

   bool operator<(const SessionContext &other) const {
       return username < other.username ||
              (username == other.username && scope < other.scope);
   }
};


std::ostream& operator<< (std::ostream& os, const SessionContext& context);

std::string sessionScopeFile(std::string prefix,
                             const SessionScope& scope);

std::string sessionScopePrefix(const std::string& username);

std::string sessionScopesPrefix(const std::string& username);

std::string sessionContextFile(const SessionContext& context);

std::string generateScopeId();
std::string generateScopeId(const std::vector<std::string>& reserved);


} // namespace r_util
} // namespace core 
} // namespace rstudio


#endif // CORE_R_UTIL_R_SESSION_CONTEXT_HPP
