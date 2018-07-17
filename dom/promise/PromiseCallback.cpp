/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PromiseCallback.h"
#include "mozilla/dom/Promise.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(PromiseCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PromiseCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PromiseCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(PromiseCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(PromiseCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

PromiseCallback::PromiseCallback()
{
  MOZ_COUNT_CTOR(PromiseCallback);
}

PromiseCallback::~PromiseCallback()
{
  MOZ_COUNT_DTOR(PromiseCallback);
}

static void
EnterCompartment(Maybe<JSAutoCompartment>& aAc, JSContext* aCx,
                 const Optional<JS::Handle<JS::Value> >& aValue)
{
  // FIXME Bug 878849
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> rooted(aCx, &aValue.Value().toObject());
    aAc.construct(aCx, rooted);
  }
}

// ResolvePromiseCallback

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(ResolvePromiseCallback,
                                     PromiseCallback,
                                     mPromise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ResolvePromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(ResolvePromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(ResolvePromiseCallback, PromiseCallback)

ResolvePromiseCallback::ResolvePromiseCallback(Promise* aPromise)
  : mPromise(aPromise)
{
  MOZ_ASSERT(aPromise);
  MOZ_COUNT_CTOR(ResolvePromiseCallback);
}

ResolvePromiseCallback::~ResolvePromiseCallback()
{
  MOZ_COUNT_DTOR(ResolvePromiseCallback);
}

void
ResolvePromiseCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  // Run resolver's algorithm with value and the synchronous flag set.
  AutoJSContext cx;
  Maybe<JSAutoCompartment> ac;
  EnterCompartment(ac, cx, aValue);

  mPromise->ResolveInternal(cx, aValue, Promise::SyncTask);
}

// RejectPromiseCallback

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(RejectPromiseCallback,
                                     PromiseCallback,
                                     mPromise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(RejectPromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(RejectPromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(RejectPromiseCallback, PromiseCallback)

RejectPromiseCallback::RejectPromiseCallback(Promise* aPromise)
  : mPromise(aPromise)
{
  MOZ_ASSERT(aPromise);
  MOZ_COUNT_CTOR(RejectPromiseCallback);
}

RejectPromiseCallback::~RejectPromiseCallback()
{
  MOZ_COUNT_DTOR(RejectPromiseCallback);
}

void
RejectPromiseCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  // Run resolver's algorithm with value and the synchronous flag set.
  AutoJSContext cx;
  Maybe<JSAutoCompartment> ac;
  EnterCompartment(ac, cx, aValue);

  mPromise->RejectInternal(cx, aValue, Promise::SyncTask);
}

// WrapperPromiseCallback

NS_IMPL_CYCLE_COLLECTION_INHERITED_2(WrapperPromiseCallback,
                                     PromiseCallback,
                                     mNextPromise, mCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(WrapperPromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(WrapperPromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(WrapperPromiseCallback, PromiseCallback)

WrapperPromiseCallback::WrapperPromiseCallback(Promise* aNextPromise,
                                               AnyCallback* aCallback)
  : mNextPromise(aNextPromise)
  , mCallback(aCallback)
{
  MOZ_ASSERT(aNextPromise);
  MOZ_COUNT_CTOR(WrapperPromiseCallback);
}

WrapperPromiseCallback::~WrapperPromiseCallback()
{
  MOZ_COUNT_DTOR(WrapperPromiseCallback);
}

void
WrapperPromiseCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  AutoJSContext cx;
  Maybe<JSAutoCompartment> ac;
  EnterCompartment(ac, cx, aValue);

  ErrorResult rv;

  Optional<JS::Handle<JS::Value> > value(cx,                
    mCallback->Call(mNextPromise, aValue, rv));

  rv.WouldReportJSException();

  if (rv.Failed() && rv.IsJSException()) {
    // If invoking callback threw an exception, run resolver's reject with the
    // thrown exception as argument and the synchronous flag set.
    Optional<JS::Handle<JS::Value> > value(cx);
    rv.StealJSException(cx, &value.Value());

    Maybe<JSAutoCompartment> ac2;
    EnterCompartment(ac2, cx, value);
    mNextPromise->RejectInternal(cx, value, Promise::SyncTask);
    return;
  }

  // Otherwise, run resolver's resolve with value and the synchronous flag
  // set.
  Maybe<JSAutoCompartment> ac2;
  EnterCompartment(ac2, cx, value);
  mNextPromise->ResolveInternal(cx, value, Promise::SyncTask);
}

// SimpleWrapperPromiseCallback

NS_IMPL_CYCLE_COLLECTION_INHERITED_2(SimpleWrapperPromiseCallback,
                                     PromiseCallback,
                                     mPromise, mCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SimpleWrapperPromiseCallback)
NS_INTERFACE_MAP_END_INHERITING(PromiseCallback)

NS_IMPL_ADDREF_INHERITED(SimpleWrapperPromiseCallback, PromiseCallback)
NS_IMPL_RELEASE_INHERITED(SimpleWrapperPromiseCallback, PromiseCallback)

SimpleWrapperPromiseCallback::SimpleWrapperPromiseCallback(Promise* aPromise,
                                                         AnyCallback* aCallback)
  : mPromise(aPromise)
  , mCallback(aCallback)
{
  MOZ_ASSERT(aPromise);
  MOZ_COUNT_CTOR(SimpleWrapperPromiseCallback);
}

SimpleWrapperPromiseCallback::~SimpleWrapperPromiseCallback()
{
  MOZ_COUNT_DTOR(SimpleWrapperPromiseCallback);
}

void
SimpleWrapperPromiseCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  ErrorResult rv;
  mCallback->Call(mPromise, aValue, rv);
}

/* static */ PromiseCallback*
PromiseCallback::Factory(Promise* aNextPromise, AnyCallback* aCallback,
                         Task aTask)
{
  MOZ_ASSERT(aNextPromise);

  // If we have a callback and a next resolver, we have to exec the callback and
  // then propagate the return value to the next resolver->resolve().
  if (aCallback) {
    return new WrapperPromiseCallback(aNextPromise, aCallback);
  }

  if (aTask == Resolve) {
    return new ResolvePromiseCallback(aNextPromise);
  }

  if (aTask == Reject) {
    return new RejectPromiseCallback(aNextPromise);
  }

  MOZ_ASSERT(false, "This should not happen");
  return nullptr;
}

} // namespace dom
} // namespace mozilla
