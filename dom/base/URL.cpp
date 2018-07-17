/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "URL.h"

#include "nsGlobalWindow.h"
#include "nsIDOMFile.h"
#include "DOMMediaStream.h"
#include "mozilla/dom/URLBinding.h"
#include "nsContentUtils.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsServiceManagerUtils.h"
#include "nsIIOService.h"
#include "nsEscape.h"
#include "nsNetCID.h"
#include "nsIURL.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_1(URL, mWindow)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(URL, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(URL, Release)

URL::URL(nsPIDOMWindow* aWindow, nsIURI* aURI)
  : mWindow(aWindow)
  , mURI(aURI)
{
}

JSObject*
URL::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return URLBinding::Wrap(aCx, aScope, this);
}

/* static */ already_AddRefed<URL>
URL::Constructor(const GlobalObject& aGlobal, const nsAString& aUrl,
                 URL& aBase, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.Get());
  if (!window) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsresult rv;
  nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  nsCOMPtr<nsIURI> uri;
  rv = ioService->NewURI(NS_ConvertUTF16toUTF8(aUrl), nullptr, aBase.GetURI(),
                         getter_AddRefs(uri));
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return nullptr;
  }

  nsRefPtr<URL> url = new URL(window, uri);
  return url.forget();
}

/* static */ already_AddRefed<URL>
URL::Constructor(const GlobalObject& aGlobal, const nsAString& aUrl,
                  const nsAString& aBase, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.Get());
  if (!window) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsresult rv;
  nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  nsCOMPtr<nsIURI> baseUri;
  rv = ioService->NewURI(NS_ConvertUTF16toUTF8(aBase), nullptr, nullptr,
                         getter_AddRefs(baseUri));
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return nullptr;
  }

  nsCOMPtr<nsIURI> uri;
  rv = ioService->NewURI(NS_ConvertUTF16toUTF8(aUrl), nullptr, baseUri,
                         getter_AddRefs(uri));
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return nullptr;
  }

  nsRefPtr<URL> url = new URL(window, uri);
  return url.forget();
}
 
void
URL::CreateObjectURL(const GlobalObject& aGlobal, nsIDOMBlob* aBlob,
                     const objectURLOptions& aOptions,
                     nsString& aResult,
                     ErrorResult& aError)
{
  CreateObjectURLInternal(aGlobal.Get(), aBlob,
                          NS_LITERAL_CSTRING(BLOBURI_SCHEME), aOptions, aResult,
                          aError);
}

void
URL::CreateObjectURL(const GlobalObject& aGlobal, DOMMediaStream& aStream,
                     const mozilla::dom::objectURLOptions& aOptions,
                     nsString& aResult,
                     ErrorResult& aError)
{
  CreateObjectURLInternal(aGlobal.Get(), &aStream,
                          NS_LITERAL_CSTRING(MEDIASTREAMURI_SCHEME), aOptions,
                          aResult, aError);
}

void
URL::CreateObjectURLInternal(nsISupports* aGlobal, nsISupports* aObject,
                             const nsACString& aScheme,
                             const objectURLOptions& aOptions,
                             nsString& aResult, ErrorResult& aError)
{
  nsCOMPtr<nsPIDOMWindow> w = do_QueryInterface(aGlobal);
  nsGlobalWindow* window = static_cast<nsGlobalWindow*>(w.get());
  NS_PRECONDITION(!window || window->IsInnerWindow(),
                  "Should be inner window");

  if (!window || !window->GetExtantDoc()) {
    aError.Throw(NS_ERROR_INVALID_POINTER);
    return;
  }

  nsIDocument* doc = window->GetExtantDoc();

  nsCString url;
  nsresult rv = nsHostObjectProtocolHandler::AddDataEntry(aScheme, aObject,
    doc->NodePrincipal(), url);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    return;
  }

  doc->RegisterHostObjectUri(url);
  CopyASCIItoUTF16(url, aResult);
}

void
URL::RevokeObjectURL(const GlobalObject& aGlobal, const nsAString& aURL)
{
  nsCOMPtr<nsPIDOMWindow> w = do_QueryInterface(aGlobal.Get());
  nsGlobalWindow* window = static_cast<nsGlobalWindow*>(w.get());
  NS_PRECONDITION(!window || window->IsInnerWindow(),
                  "Should be inner window");
  if (!window)
    return;

  NS_LossyConvertUTF16toASCII asciiurl(aURL);

  nsIPrincipal* winPrincipal = window->GetPrincipal();
  if (!winPrincipal) {
    return;
  }

  nsIPrincipal* principal =
    nsHostObjectProtocolHandler::GetDataEntryPrincipal(asciiurl);
  bool subsumes;
  if (principal && winPrincipal &&
      NS_SUCCEEDED(winPrincipal->Subsumes(principal, &subsumes)) &&
      subsumes) {
    if (window->GetExtantDoc()) {
      window->GetExtantDoc()->UnregisterHostObjectUri(asciiurl);
    }
    nsHostObjectProtocolHandler::RemoveDataEntry(asciiurl);
  }
}

void
URL::GetHref(nsString& aHref) const
{
  aHref.Truncate();

  nsAutoCString href;
  nsresult rv = mURI->GetSpec(href);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(href, aHref);
  }
}

void
URL::SetHref(const nsAString& aHref, ErrorResult& aRv)
{
  aRv = mURI->SetSpec(NS_ConvertUTF16toUTF8(aHref));
}

void
URL::GetOrigin(nsString& aOrigin) const
{
  nsContentUtils::GetUTFNonNullOrigin(mURI, aOrigin);
}

void
URL::GetProtocol(nsString& aProtocol) const
{
  nsCString protocol;
  if (NS_SUCCEEDED(mURI->GetScheme(protocol))) {
    aProtocol.Truncate();
  }

  CopyASCIItoUTF16(protocol, aProtocol);
  aProtocol.Append(PRUnichar(':'));
}

void
URL::SetProtocol(const nsAString& aProtocol)
{
  nsAString::const_iterator start, end;
  aProtocol.BeginReading(start);
  aProtocol.EndReading(end);
  nsAString::const_iterator iter(start);

  FindCharInReadable(':', iter, end);
  mURI->SetScheme(NS_ConvertUTF16toUTF8(Substring(start, iter)));
}

#define URL_GETTER( value, func ) \
  value.Truncate();               \
  nsAutoCString tmp;              \
  nsresult rv = mURI->func(tmp);  \
  if (NS_SUCCEEDED(rv)) {         \
    CopyUTF8toUTF16(tmp, value);  \
  }

void
URL::GetUsername(nsString& aUsername) const
{
  URL_GETTER(aUsername, GetUsername);
}

void
URL::SetUsername(const nsAString& aUsername)
{
  mURI->SetUsername(NS_ConvertUTF16toUTF8(aUsername));
}

void
URL::GetPassword(nsString& aPassword) const
{
  URL_GETTER(aPassword, GetPassword);
}

void
URL::SetPassword(const nsAString& aPassword)
{
  mURI->SetPassword(NS_ConvertUTF16toUTF8(aPassword));
}

void
URL::GetHost(nsString& aHost) const
{
  URL_GETTER(aHost, GetHostPort);
}

void
URL::SetHost(const nsAString& aHost)
{
  mURI->SetHostPort(NS_ConvertUTF16toUTF8(aHost));
}

void
URL::GetHostname(nsString& aHostname) const
{
  URL_GETTER(aHostname, GetHost);
}

void
URL::SetHostname(const nsAString& aHostname)
{
  mURI->SetHost(NS_ConvertUTF16toUTF8(aHostname));
}

void
URL::GetPort(nsString& aPort) const
{
  aPort.Truncate();

  int32_t port;
  nsresult rv = mURI->GetPort(&port);
  if (NS_SUCCEEDED(rv) && port != -1) {
    nsAutoString portStr;
    portStr.AppendInt(port, 10);
    aPort.Assign(portStr);
  }
}

void
URL::SetPort(const nsAString& aPort)
{
  nsresult rv;
  nsAutoString portStr(aPort);
  int32_t port = portStr.ToInteger(&rv);
  if (NS_FAILED(rv)) {
    return;
  }

  mURI->SetPort(port);
}

void
URL::GetPathname(nsString& aPathname) const
{
  aPathname.Truncate();

  nsCOMPtr<nsIURL> url(do_QueryInterface(mURI));
  if (!url) {
    // Do not throw!  Not having a valid URI or URL should result in an empty
    // string.
    return;
  }

  nsAutoCString file;
  nsresult rv = url->GetFilePath(file);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(file, aPathname);
  }
}

void
URL::SetPathname(const nsAString& aPathname)
{
  nsCOMPtr<nsIURL> url(do_QueryInterface(mURI));
  if (!url) {
    // Ignore failures to be compatible with NS4.
    return;
  }

  url->SetFilePath(NS_ConvertUTF16toUTF8(aPathname));
}

void
URL::GetSearch(nsString& aSearch) const
{
  aSearch.Truncate();

  nsCOMPtr<nsIURL> url(do_QueryInterface(mURI));
  if (!url) {
    // Do not throw!  Not having a valid URI or URL should result in an empty
    // string.
    return;
  }

  nsAutoCString search;
  nsresult rv = url->GetQuery(search);
  if (NS_SUCCEEDED(rv) && !search.IsEmpty()) {
    CopyUTF8toUTF16(NS_LITERAL_CSTRING("?") + search, aSearch);
  }
}

void
URL::SetSearch(const nsAString& aSearch)
{
  nsCOMPtr<nsIURL> url(do_QueryInterface(mURI));
  if (!url) {
    // Ignore failures to be compatible with NS4.
    return;
  }

  url->SetQuery(NS_ConvertUTF16toUTF8(aSearch));
}

void
URL::GetHash(nsString& aHash) const
{
  aHash.Truncate();

  nsAutoCString ref;
  nsresult rv = mURI->GetRef(ref);
  if (NS_SUCCEEDED(rv) && !ref.IsEmpty()) {
    NS_UnescapeURL(ref); // XXX may result in random non-ASCII bytes!
    aHash.Assign(PRUnichar('#'));
    AppendUTF8toUTF16(ref, aHash);
  }
}

void
URL::SetHash(const nsAString& aHash)
{
  mURI->SetRef(NS_ConvertUTF16toUTF8(aHash));
}

}
}
