/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prlog.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIChannelPolicy.h"
#include "nsIChannelEventSink.h"
#include "nsIPropertyBag2.h"
#include "nsIWritablePropertyBag2.h"
//#include "nsNetError.h"
#include "nsChannelProperties.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "mozilla/Preferences.h"
#include "nsIHttpChannel.h"
#include "nsDocument.h"
#include "nsXSSUtils.h"
#include "nsScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsISupportsPrimitives.h"
#include "nsEscape.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeItem.h"
//#include "nsContentErrors.h"
#include "nsXSSFilter.h"
#include "nsIInputStream.h"
#include "nsISeekableStream.h"
#include "nsIUploadChannel.h"
#include "nsIConsoleService.h"
#include "nsIMutableArray.h"

using namespace mozilla;

bool nsXSSFilter::sXSSEnabled = true;
bool nsXSSFilter::sReportOnly = false;
bool nsXSSFilter::sBlockMode = false;
bool nsXSSFilter::sBlockDynamic = true;

#ifdef PR_LOGGING
static PRLogModuleInfo *gXssPRLog;
#endif



nsXSSFilter::nsXSSFilter(nsIDocument *parent)
  : mParams(),
    mParamsInitialized(false),
    mParentDoc(parent),
    mDomainCache(),
    mIsEnabled(true),
    mBlockMode(false)
{ }

nsXSSFilter::~nsXSSFilter()
{ }


void
nsXSSFilter::InitializeStatics()
{
  nsXSSUtils::InitializeStatics();
#ifdef PR_LOGGING
  if (!gXssPRLog)
    gXssPRLog = PR_NewLogModule("XSS");
#endif
  Preferences::AddBoolVarCache(&sXSSEnabled, "security.xssfilter.enable");
  Preferences::AddBoolVarCache(&sReportOnly, "security.xssfilter.reportOnly");
  Preferences::AddBoolVarCache(&sBlockMode, "security.xssfilter.blockMode");
  Preferences::AddBoolVarCache(&sBlockDynamic, "security.xssfilter.blockDynamic");
  LOG_XSS("Initialized Statics for XSS Filter");
}


/**
 * Two Utility functions to parse the X-XSS-Protection header.
 * Note: the code for parsing the header purposely copies Webkit.
 * Returns true if there is more to parse.
 */
bool
skipWhiteSpace(const nsACString& str, PRUint32& pos,
               bool fromHttpEquivMeta)
{
  PRUint32 len = str.Length();

  if (fromHttpEquivMeta) {
    while (pos != len && str[pos] <= ' ')
      ++pos;
  } else {
    while (pos != len && (str[pos] == '\t' || str[pos] == ' '))
      ++pos;
  }
  return pos != len;
}

/**
 * Returns true if the function can match the whole token (case insensitive).
 * Note: Might return pos == str.Length()
 */
bool
skipToken(const nsACString& str, PRUint32& pos,
          const nsACString& token)
{
  PRUint32 len = str.Length();
  PRUint32 tokenPos = 0;
	PRUint32 tokenLen = token.Length();

  while (pos != len && tokenPos < tokenLen) {
    if (tolower(str[pos]) != token[tokenPos++]) {
      return false;
    }
    ++pos;
  }

  return true;
}

nsresult
nsXSSFilter::ScanRequestData()
{
  if (!mParentDoc->GetChannel()) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIHttpChannel> httpChannel =
    do_QueryInterface(mParentDoc->GetChannel());
  if (!httpChannel) {
    return NS_ERROR_FAILURE;
  }
  LOG_XSS_CALL("ScanRequestData");

  // Webkit logic (the de facto standard we want to comply with):
  // 1. strip leading whitespaces.
  // 2. if the first char is 0, disable the filter
  // 3. if the first char is 1 enabled the filter
  // 4. if it is "1[ ]*mode[ ]*=[ ]*block$", then enabled in block mode
  // https://bugs.webkit.org/show_bug.cgi?id=27312
  nsAutoCString xssHeaderValue;
  httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("X-Xss-Protection"),
                                 xssHeaderValue);
  LOG_XSS_1("Header: '%s'", xssHeaderValue.get());

  // No need to skip spaces before the beginning of the string; 
  // the header parser does this for us.

  if (xssHeaderValue.IsEmpty()) {
    // If the header is missing, assume the filter should be enabled.
    mIsEnabled = true;
    return NS_OK;
  }
  
  if (xssHeaderValue[0] == '0') {
    // Explicity disabled by the web server.
    mIsEnabled = false;
    return NS_OK;
  }

  PRUint32 len = xssHeaderValue.Length();
  PRUint32 pos = 0;

  if (xssHeaderValue[pos++] == '1' &&
      skipWhiteSpace(xssHeaderValue, pos, false) &&
      xssHeaderValue[pos++] == ';' &&
      skipWhiteSpace(xssHeaderValue, pos, false) &&
      skipToken(xssHeaderValue, pos, NS_LITERAL_CSTRING("mode")) &&
      skipWhiteSpace(xssHeaderValue, pos, false) &&
      xssHeaderValue[pos++] == '=' &&
      skipWhiteSpace(xssHeaderValue, pos, false) &&
      skipToken(xssHeaderValue, pos, NS_LITERAL_CSTRING("block")) &&
      pos == len) {
    mIsEnabled = true;
    mBlockMode = true;
    LOG_XSS("Block mode activated");
    return NS_OK;
  }

  // Any other value is invalid, so act as if the header was missing.
  mIsEnabled = true;
  return NS_OK;
}

bool
nsXSSFilter::PermitsInlineScript(const nsAString& aScript)
{
  LOG_XSS_CALL("Inline");
  LOG_XSS_1("script: %s",
            NS_ConvertUTF16toUTF8(Substring(aScript, 0, 70)).get());

  if (!IsEnabled()) {
    return true;
  }

  nsXSSUtils::CheckInline(aScript, GetParams());

  if (nsXSSUtils::HasAttack(GetParams())) {
    NotifyViolation(NS_LITERAL_STRING("Inline Script"), aScript, GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsExternalScript(nsIURI *aURI, bool isDynamic)
{
  if (!aURI) {
    return true;
  }

#ifdef PR_LOGGING
  LOG_XSS_CALL("External");
  nsAutoCString spec;
  aURI->GetSpec(spec);
  LOG_XSS_1("script URI: %s", spec.get());
#endif

  if (!IsEnabled()) {
    return true;
  }

  if (isDynamic && !IsBlockDynamic()) {
    return true;
  }
  // Fetch value from cache.
  bool c;
  nsAutoString domain;
  DomainMap& cache = GetDomainCache();
  nsXSSUtils::GetDomain(aURI, domain);
  if (cache.Get(domain, &c)) {
    return c;
  }

  nsXSSUtils::CheckExternal(aURI, GetURI(), GetParams());
  if (nsXSSUtils::HasAttack(GetParams())) {
    nsAutoCString spec;
    aURI->GetSpec(spec);
    NotifyViolation(NS_LITERAL_STRING("External Script"), 
                    NS_ConvertUTF8toUTF16(spec), GetURI());
    cache.Put(domain, false);
    return IsReportOnly();
  }
  cache.Put(domain, true);
  return true;
}

bool
nsXSSFilter::PermitsJSUrl(const nsAString& aURI)
{
  LOG_XSS_CALL("JSUrl");
  LOG_XSS_1("javascript url: %s",
            NS_ConvertUTF16toUTF8(Substring(aURI, 0, 70)).get());

  if (!IsEnabled()) {
    return true;
  }

  nsAutoString escUri;
  nsXSSUtils::UnescapeLoop(aURI, escUri, mParentDoc);
  LOG_XSS_1("escaped javascript url: %s",
            NS_ConvertUTF16toUTF8(escUri).get());

  nsXSSUtils::CheckInline(escUri, GetParams());

  if (nsXSSUtils::HasAttack(GetParams())) {
    NotifyViolation(NS_LITERAL_STRING("JS URL"), aURI, GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsEventListener(const nsAString& aScript)
{
  LOG_XSS_CALL("Event");
  LOG_XSS_1("Event: %s",
            NS_ConvertUTF16toUTF8(Substring(aScript, 0, 70)).get());

  if (!IsEnabled()) {
    return true;
  }

  nsXSSUtils::CheckInline(aScript, GetParams());

  if (nsXSSUtils::HasAttack(GetParams())) {
    NotifyViolation(NS_LITERAL_STRING("Event Listener"), aScript, GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsBaseElement(nsIURI *aOldURI, nsIURI* aNewURI)
{
  if (!aOldURI || !aNewURI) {
    return true;
  }

#ifdef PR_LOGGING
  LOG_XSS_CALL("Base");
  nsAutoCString spec;
  aNewURI->GetSpec(spec);
  LOG_XSS_1("new URI: %s", spec.get());
#endif

  if (!IsEnabled()) {
    return true;
  }

  // Allow the base element to change the base url on the same
  // registered domain.
  nsAutoString oldD, newD;
  nsXSSUtils::GetDomain(aOldURI, oldD);
  nsXSSUtils::GetDomain(aNewURI, newD);
  if (oldD.Equals(newD)) {
    return true;
  }

  nsXSSUtils::CheckExternal(aNewURI, GetURI(), GetParams());
  if (nsXSSUtils::HasAttack(GetParams())) {
    nsAutoCString spec;
    aNewURI->GetSpec(spec);
    NotifyViolation(NS_LITERAL_STRING("Base Element"), 
                    NS_ConvertUTF8toUTF16(spec), GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsExternalObject(nsIURI *aURI)
{
  if (!aURI) {
    return true;
  }

#ifdef PR_LOGGING
  LOG_XSS_CALL("Object");
  nsAutoCString spec;
  aURI->GetSpec(spec);
  LOG_XSS_1("object URI: %s", spec.get());
#endif

  if (!IsEnabled()) {
    return true;
  }

  // Fetch value from cache.
  bool c;
  nsAutoString domain;
  DomainMap& cache = GetDomainCache();
  nsXSSUtils::GetDomain(aURI, domain);
  if (cache.Get(domain, &c)) {
    return c;
  }

  nsXSSUtils::CheckExternal(aURI, GetURI(), GetParams());
  if (nsXSSUtils::HasAttack(GetParams())) {
    nsAutoCString spec;
    aURI->GetSpec(spec);
    NotifyViolation(NS_LITERAL_STRING("Object"),
                    NS_ConvertUTF8toUTF16(spec), GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsDataURL(nsIURI *aURI)
{
  if (!aURI) {
    return true;
  }

  nsAutoCString spec;
  aURI->GetSpec(spec);
  LOG_XSS_CALL("DataURL");
  LOG_XSS_1("data URL: %s", spec.get());

  if (!IsEnabled()) {
    return true;
  }

  nsXSSUtils::CheckInline(NS_ConvertUTF8toUTF16(spec), GetParams());

  if (nsXSSUtils::HasAttack(GetParams())) {
    NotifyViolation(NS_LITERAL_STRING("Data URL"),
                    NS_ConvertUTF8toUTF16(spec), GetURI());
    return IsReportOnly();
  }
  return true;
}

bool
nsXSSFilter::PermitsJSAction(const nsAString& aCode)
{
  LOG_XSS_CALL("JSAction");
  LOG_XSS_1("JS: %s", NS_ConvertUTF16toUTF8(Substring(aCode, 0, 100)).get());

  if (!IsEnabled()) {
    return true;
  }

  nsXSSUtils::CheckInline(aCode, GetParams());

  if (nsXSSUtils::HasAttack(GetParams())) {
    NotifyViolation(NS_LITERAL_STRING("JS Action"), aCode, GetURI());
    return IsReportOnly();
  }
  return true;
}

ParameterArray&
nsXSSFilter::GetParams()
{
  if (!mParamsInitialized) {

    // Get params
    nsXSSUtils::ParseURI(GetURI(), mParams, mParentDoc);

    // Post params
    nsCOMPtr<nsIHttpChannel> httpChannel =
      do_QueryInterface(mParentDoc->GetChannel());
    nsAutoCString method;
    httpChannel->GetRequestMethod(method);
    if (method.EqualsLiteral("POST")) {
      nsCOMPtr<nsIUploadChannel> uploadChannel =
        do_QueryInterface(httpChannel);
      nsCOMPtr<nsIInputStream> uploadStream;
      uploadChannel->GetUploadStream(getter_AddRefs(uploadStream));
      // Rewind the stream.
      nsCOMPtr<nsISeekableStream> seekStream = do_QueryInterface(uploadStream);
      seekStream->Seek(nsISeekableStream::NS_SEEK_SET, 0);

      // TODO: this was 32-bit before
      uint64_t len;
      uploadStream->Available(&len);
      char* buf = static_cast<char*>(moz_xmalloc(len+1));
      PRUint32 bytesRead;
      uploadStream->Read(buf, len, &bytesRead);
      if (bytesRead != len) {
        free(buf);
      }
      buf[len] = '\0';
      nsXSSUtils::ParsePOST(buf, mParams, mParentDoc);
      free(buf);
    }

    mParamsInitialized = true;

  }

  return mParams;
}

nsIURI*
nsXSSFilter::GetURI()
{
  return mParentDoc->GetDocumentURI();
}

DomainMap&
nsXSSFilter::GetDomainCache()
{
  if (!mDomainCache.IsInitialized()) {
    mDomainCache.Init();
  }
  return mDomainCache;
}

bool nsXSSFilter::IsEnabled() { return sXSSEnabled && mIsEnabled; }
bool nsXSSFilter::IsBlockMode() { return sBlockMode || mBlockMode; }
bool nsXSSFilter::IsReportOnly() { return sReportOnly; }
bool nsXSSFilter::IsBlockDynamic() { return sBlockDynamic; }


class nsXSSNotifier: public nsRunnable
{
public:
  nsXSSNotifier(const nsAString& policy, const nsAString& content, nsIURI* url, bool blockMode) :
    mPolicy(policy), mContent(content),  mURI(url), mBlockMode(blockMode)
  { };

  NS_IMETHOD Run() {
    LOG_XSS("Sending Observer Notification");
    nsCOMPtr<nsIObserverService> observerService =
      services::GetObserverService();
    if (!observerService) {
      return NS_OK;
    }

    // The nsIArray will contain four parameters:
    // violated policy, content, url and blockMode
    nsresult rv = NS_OK;
    nsCOMPtr<nsIMutableArray> params = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsCString>
      wrappedPolicy(do_CreateInstance("@mozilla.org/supports-cstring;1"));
    wrappedPolicy->SetData(NS_ConvertUTF16toUTF8(mPolicy));

    nsCOMPtr<nsISupportsCString>
      wrappedContent(do_CreateInstance("@mozilla.org/supports-cstring;1"));
    wrappedContent->SetData(NS_ConvertUTF16toUTF8(mContent));

    nsCOMPtr<nsISupportsCString>
      wrappedUrl(do_CreateInstance("@mozilla.org/supports-cstring;1"));
    nsAutoCString spec;
    mURI->GetSpec(spec);
    wrappedUrl->SetData(spec);

    nsCOMPtr<nsISupportsPRBool>
      wrappedBlock(do_CreateInstance("@mozilla.org/supports-PRBool;1"));
    wrappedBlock->SetData(mBlockMode);
    
    params->AppendElement(wrappedPolicy, false);
    params->AppendElement(wrappedContent, false);
    params->AppendElement(wrappedUrl, false);
    params->AppendElement(wrappedBlock, false);
    observerService->NotifyObservers(params, "xss-on-violate-policy", NULL);
    return NS_OK;
  }



private:
  ~nsXSSNotifier() { };
  nsString mPolicy, mContent;
  nsIURI* mURI;
  bool mBlockMode;
};

nsresult
nsXSSFilter::NotifyViolation(const nsAString& policy, const nsAString& content, nsIURI* url)
{
  LOG_XSS("Violation");

  nsAutoCString spec;
  url->GetSpec(spec);

  // Send to console.
  nsCOMPtr<nsIConsoleService> aConsoleService =
    do_GetService( "@mozilla.org/consoleservice;1" );
  nsAutoString msg;
  msg.Assign(NS_LITERAL_STRING("XSS Violation on URL "));
  msg.Append(NS_ConvertUTF8toUTF16(spec));
  msg.Append(NS_LITERAL_STRING(": "));
  msg.Append(policy);
  aConsoleService->
    LogStringMessage(msg.get());

  // Send to observers as xss-on-violate-policy.
  nsCOMPtr<nsIThread> thread = do_GetMainThread();
  if (!thread) {
    LOG_XSS("Main thread unavailable");
    return NS_OK;
  }
  nsCOMPtr<nsIRunnable> runnable = new nsXSSNotifier(policy, content, url, IsBlockMode());
  thread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);

  // Block the page load if block mode is enabled.
  if (IsBlockMode()) {
    nsCOMPtr<nsIHttpChannel> httpChannel =
      do_QueryInterface(mParentDoc->GetChannel());
    httpChannel->Cancel(NS_ERROR_XSS_BLOCK);
  }

  return NS_OK;
}
