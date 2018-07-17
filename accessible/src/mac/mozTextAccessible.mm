/* -*- Mode: Objective-C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Accessible-inl.h"
#include "AccessibleWrap.h"
#include "TextLeafAccessible.h"

#include "nsCocoaUtils.h"
#include "nsObjCExceptions.h"

#import "mozTextAccessible.h"

using namespace mozilla::a11y;

inline bool
ToNSRange(id aValue, NSRange* aRange)
{
  NS_PRECONDITION(aRange, "aRange is nil");

  if ([aValue isKindOfClass:[NSValue class]] && 
      strcmp([(NSValue*)aValue objCType], @encode(NSRange)) == 0) {
    *aRange = [aValue rangeValue];
    return true;
  }

  return false;
}

inline NSString*
ToNSString(id aValue)
{
  if ([aValue isKindOfClass:[NSString class]]) {
    return aValue;
  }

  return nil;
}

@interface mozTextAccessible ()
- (NSString*)subrole;
- (NSString*)selectedText;
- (NSValue*)selectedTextRange;
- (NSValue*)visibleCharacterRange;
- (long)textLength;
- (BOOL)isReadOnly;
- (NSNumber*)caretLineNumber;
- (void)setText:(NSString*)newText;
- (NSString*)text;
- (NSString*)stringFromRange:(NSRange*)range;
@end

@implementation mozTextAccessible

- (id)initWithAccessible:(AccessibleWrap*)accessible
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  if ((self = [super initWithAccessible:accessible])) {
    mGoannaTextAccessible = accessible->AsHyperText();
    CallQueryInterface(accessible, &mGoannaEditableTextAccessible);
  }
  return self;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (BOOL)accessibilityIsIgnored
{
  return !mGoannaAccessible;
}

- (NSArray*)accessibilityAttributeNames
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  static NSMutableArray* supportedAttributes = nil;
  if (!supportedAttributes) {
    // text-specific attributes to supplement the standard one
    supportedAttributes = [[NSMutableArray alloc] initWithObjects:
      NSAccessibilitySelectedTextAttribute, // required
      NSAccessibilitySelectedTextRangeAttribute, // required
      NSAccessibilityNumberOfCharactersAttribute, // required
      NSAccessibilityVisibleCharacterRangeAttribute, // required
      NSAccessibilityInsertionPointLineNumberAttribute,
      @"AXRequired",
      @"AXInvalid",
      nil
    ];
    [supportedAttributes addObjectsFromArray:[super accessibilityAttributeNames]];
  }
  return supportedAttributes;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (id)accessibilityAttributeValue:(NSString*)attribute
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  if ([attribute isEqualToString:NSAccessibilityNumberOfCharactersAttribute])
    return [NSNumber numberWithInt:[self textLength]];

  if ([attribute isEqualToString:NSAccessibilityInsertionPointLineNumberAttribute])
    return [self caretLineNumber];

  if ([attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute])
    return [self selectedTextRange];

  if ([attribute isEqualToString:NSAccessibilitySelectedTextAttribute])
    return [self selectedText];

  if ([attribute isEqualToString:NSAccessibilityTitleAttribute])
    return @"";

  if ([attribute isEqualToString:NSAccessibilityValueAttribute]) {
    // Apple's SpeechSynthesisServer expects AXValue to return an AXStaticText
    // object's AXSelectedText attribute. See bug 674612 for details.
    // Also if there is no selected text, we return the full text. 
    // See bug 369710 for details.
    if ([[self role] isEqualToString:NSAccessibilityStaticTextRole]) {
      NSString* selectedText = [self selectedText];
      return (selectedText && [selectedText length]) ? selectedText : [self text];
    }

    return [self text];
  }

  if ([attribute isEqualToString:@"AXRequired"])
    return [NSNumber numberWithBool:!!(mGoannaAccessible->State() & states::REQUIRED)];

  if ([attribute isEqualToString:@"AXInvalid"])
    return [NSNumber numberWithBool:!!(mGoannaAccessible->State() & states::INVALID)];

  if ([attribute isEqualToString:NSAccessibilityVisibleCharacterRangeAttribute])
    return [self visibleCharacterRange];

  // let mozAccessible handle all other attributes
  return [super accessibilityAttributeValue:attribute];

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (NSArray*)accessibilityParameterizedAttributeNames
{
  static NSArray* supportedParametrizedAttributes = nil;
  // text specific parametrized attributes
  if (!supportedParametrizedAttributes) {
    supportedParametrizedAttributes = [[NSArray alloc] initWithObjects:
      NSAccessibilityStringForRangeParameterizedAttribute,
      NSAccessibilityLineForIndexParameterizedAttribute,
      NSAccessibilityRangeForLineParameterizedAttribute,
      NSAccessibilityAttributedStringForRangeParameterizedAttribute,
      NSAccessibilityBoundsForRangeParameterizedAttribute,
#if DEBUG
      NSAccessibilityRangeForPositionParameterizedAttribute,
      NSAccessibilityRangeForIndexParameterizedAttribute,
      NSAccessibilityRTFForRangeParameterizedAttribute,
      NSAccessibilityStyleRangeForIndexParameterizedAttribute,
#endif
      nil
    ];
  }
  return supportedParametrizedAttributes;
}

- (id)accessibilityAttributeValue:(NSString*)attribute forParameter:(id)parameter
{
  if (!mGoannaTextAccessible)
    return nil;

  if ([attribute isEqualToString:NSAccessibilityStringForRangeParameterizedAttribute]) {
    NSRange range;
    if (!ToNSRange(parameter, &range)) {
#if DEBUG
      NSLog(@"%@: range not set", attribute);
#endif
      return @"";
    }

    return [self stringFromRange:&range];
  }

  if ([attribute isEqualToString:NSAccessibilityRangeForLineParameterizedAttribute]) {
    // XXX: actually get the integer value for the line #
    return [NSValue valueWithRange:NSMakeRange(0, [self textLength])];
  }

  if ([attribute isEqualToString:NSAccessibilityAttributedStringForRangeParameterizedAttribute]) {
    NSRange range;
    if (!ToNSRange(parameter, &range)) {
#if DEBUG
      NSLog(@"%@: range not set", attribute);
#endif
      return @"";
    }

    return [[[NSAttributedString alloc] initWithString:[self stringFromRange:&range]] autorelease];
  }

  if ([attribute isEqualToString:NSAccessibilityLineForIndexParameterizedAttribute]) {
    // XXX: actually return the line #
    return [NSNumber numberWithInt:0];
  }

  if ([attribute isEqualToString:NSAccessibilityBoundsForRangeParameterizedAttribute]) {
    NSRange range;
    if (!ToNSRange(parameter, &range)) {
#if DEBUG
      NSLog(@"%@:no range", attribute);
#endif
      return nil;
    }
    
    int32_t start = range.location;
    int32_t end = start + range.length;
    nsIntRect bounds = mGoannaTextAccessible->GetTextBounds(start, end);

    return [NSValue valueWithRect:nsCocoaUtils::GoannaRectToCocoaRect(bounds)];
  }

#if DEBUG
  NSLog(@"unhandled attribute:%@ forParameter:%@", attribute, parameter);
#endif

  return nil;
}

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  if ([attribute isEqualToString:NSAccessibilityValueAttribute])
    return ![self isReadOnly];
  
  if ([attribute isEqualToString:NSAccessibilitySelectedTextAttribute] ||
      [attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute] ||
      [attribute isEqualToString:NSAccessibilityVisibleCharacterRangeAttribute])
    return YES;

  return [super accessibilityIsAttributeSettable:attribute];

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(NO);
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  if (!mGoannaTextAccessible)
    return;

  if ([attribute isEqualToString:NSAccessibilityValueAttribute]) {
    [self setText:ToNSString(value)];
    
    return;
  }

  if ([attribute isEqualToString:NSAccessibilitySelectedTextAttribute]) {
    NSString* stringValue = ToNSString(value);
    if (!stringValue)
      return;

    int32_t start = 0;
    int32_t end = 0;

    nsresult rv = mGoannaTextAccessible->GetSelectionBounds(0, &start, &end);
    NS_ENSURE_SUCCESS(rv,);
    
    rv = mGoannaTextAccessible->DeleteText(start, end - start);
    NS_ENSURE_SUCCESS(rv,);

    nsString text;
    nsCocoaUtils::GetStringForNSString(stringValue, text);
    rv = mGoannaTextAccessible->InsertText(text, start);
    NS_ENSURE_SUCCESS(rv,);

    return;
  }

  if ([attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute]) {
    NSRange range;
    if (!ToNSRange(value, &range))
      return;

    nsresult rv = mGoannaTextAccessible->SetSelectionBounds(0, range.location, 
                                                           range.location + range.length);
    NS_ENSURE_SUCCESS(rv,);

    return;
  }

  if ([attribute isEqualToString:NSAccessibilityVisibleCharacterRangeAttribute]) {
    NSRange range;
    if (!ToNSRange(value, &range))
      return;

    mGoannaTextAccessible->ScrollSubstringTo(range.location, range.location + range.length,
                                            nsIAccessibleScrollType::SCROLL_TYPE_TOP_EDGE);
    return;
  } 

  [super accessibilitySetValue:value forAttribute:attribute];

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (NSString*)subrole
{
  if(mRole == roles::PASSWORD_TEXT)
    return NSAccessibilitySecureTextFieldSubrole;

  return nil;
}

- (void)expire
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  mGoannaTextAccessible = nullptr;
  NS_IF_RELEASE(mGoannaEditableTextAccessible);
  [super expire];

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

#pragma mark -

- (BOOL)isReadOnly
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  if ([[self role] isEqualToString:NSAccessibilityStaticTextRole])
    return YES;
    
  if (mGoannaEditableTextAccessible)
    return (mGoannaAccessible->State() & states::READONLY) == 0;

  return NO;

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(NO);
}

- (NSNumber*)caretLineNumber
{
  int32_t lineNumber = mGoannaTextAccessible ?
    mGoannaTextAccessible->CaretLineNumber() - 1 : -1;

  return (lineNumber >= 0) ? [NSNumber numberWithInt:lineNumber] : nil;
}

- (void)setText:(NSString*)aNewString
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  if (mGoannaEditableTextAccessible) {
    nsString text;
    nsCocoaUtils::GetStringForNSString(aNewString, text);
    mGoannaEditableTextAccessible->SetTextContents(text);
  }

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (NSString*)text
{
  if (!mGoannaAccessible || !mGoannaTextAccessible)
    return nil;

  // A password text field returns an empty value
  if (mRole == roles::PASSWORD_TEXT)
    return @"";

  nsAutoString text;
  nsresult rv = mGoannaTextAccessible->
    GetText(0, nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT, text);
  NS_ENSURE_SUCCESS(rv, @"");

  return nsCocoaUtils::ToNSString(text);
}

- (long)textLength
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  if (!mGoannaAccessible || !mGoannaTextAccessible)
    return 0;

  return mGoannaTextAccessible ? mGoannaTextAccessible->CharacterCount() : 0;

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(0);
}

- (long)selectedTextLength
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

  if (mGoannaTextAccessible) {
    int32_t start, end;
    start = end = 0;
    nsresult rv = mGoannaTextAccessible->GetSelectionBounds(0, &start, &end);
    NS_ENSURE_SUCCESS(rv, 0);

    return (end - start);
  }
  return 0;

  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(0);
}

- (NSString*)selectedText
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  if (mGoannaTextAccessible) {
    int32_t start, end;
    start = end = 0;
    mGoannaTextAccessible->GetSelectionBounds(0, &start, &end);
    if (start != end) {
      nsAutoString selText;
      mGoannaTextAccessible->GetText(start, end, selText);
      return nsCocoaUtils::ToNSString(selText);
    }
  }
  return nil;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (NSValue*)selectedTextRange
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  if (mGoannaTextAccessible) {
    int32_t start = 0;
    int32_t end = 0;
    int32_t count = 0;

    nsresult rv = mGoannaTextAccessible->GetSelectionCount(&count);
    NS_ENSURE_SUCCESS(rv, nil);

    if (count) {
      rv = mGoannaTextAccessible->GetSelectionBounds(0, &start, &end);
      NS_ENSURE_SUCCESS(rv, nil);

      return [NSValue valueWithRange:NSMakeRange(start, end - start)];
    }

    rv = mGoannaTextAccessible->GetCaretOffset(&start);
    NS_ENSURE_SUCCESS(rv, nil);
    
    return [NSValue valueWithRange:NSMakeRange(start != -1 ? start : 0, 0)]; 
  }
  return [NSValue valueWithRange:NSMakeRange(0, 0)];

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

- (NSValue*)visibleCharacterRange
{
  // XXX this won't work with Textarea and such as we actually don't give
  // the visible character range.
  return [NSValue valueWithRange:
    NSMakeRange(0, mGoannaTextAccessible ? 
                mGoannaTextAccessible->CharacterCount() : 0)];
}

- (void)valueDidChange
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  NSAccessibilityPostNotification(GetObjectOrRepresentedView(self),
                                  NSAccessibilityValueChangedNotification);

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

- (void)selectedTextDidChange
{
  NSAccessibilityPostNotification(GetObjectOrRepresentedView(self),
                                  NSAccessibilitySelectedTextChangedNotification);
}

- (NSString*)stringFromRange:(NSRange*)range
{
  NS_PRECONDITION(mGoannaTextAccessible && range, "no Goanna text accessible or range");

  nsAutoString text;
  mGoannaTextAccessible->GetText(range->location, 
                                range->location + range->length, text);
  return nsCocoaUtils::ToNSString(text);
}

@end

@implementation mozTextLeafAccessible

- (NSArray*)accessibilityAttributeNames
{
  static NSMutableArray* supportedAttributes = nil;
  if (!supportedAttributes) {
    supportedAttributes = [[super accessibilityAttributeNames] mutableCopy];
    [supportedAttributes removeObject:NSAccessibilityChildrenAttribute];
  }

  return supportedAttributes;
}

- (id)accessibilityAttributeValue:(NSString*)attribute
{
  if ([attribute isEqualToString:NSAccessibilityTitleAttribute])
    return @"";

  if ([attribute isEqualToString:NSAccessibilityValueAttribute])
    return [self text];

  return [super accessibilityAttributeValue:attribute];
}

- (NSString*)text
{
  if (!mGoannaAccessible)
    return nil;

  return nsCocoaUtils::ToNSString(mGoannaAccessible->AsTextLeaf()->Text());
}

- (long)textLength
{
  if (!mGoannaAccessible)
    return 0;

  return mGoannaAccessible->AsTextLeaf()->Text().Length();
}

@end
