/*
 *  spotlight.c
 *  spotlight
 *
 *  Created by xli on 4/26/10.
 *  Copyright 2010 ThoughtWorks. All rights reserved.
 */

#include <ruby.h>
#include <CoreServices/CoreServices.h>

#define RELEASE_IF_NOT_NULL(ref) { if (ref) { CFRelease(ref); } }

void MDItemSetAttribute(MDItemRef item, CFStringRef name, CFTypeRef value);

VALUE method_search(VALUE self, VALUE queryString, VALUE scopeDirectory);
VALUE method_attributes(VALUE self, VALUE path);
VALUE method_set_attribute(VALUE self, VALUE path, VALUE name, VALUE value);
VALUE method_get_attribute(VALUE self, VALUE path, VALUE name);

void Init_spotlight (void)
{
  VALUE Spotlight = rb_define_module("Spotlight");
  VALUE SpotlightIntern = rb_define_module_under(Spotlight, "Intern");
  rb_define_module_function(SpotlightIntern, "search", method_search, 2);
  rb_define_module_function(SpotlightIntern, "attributes", method_attributes, 1);
  rb_define_module_function(SpotlightIntern, "set_attribute", method_set_attribute, 3);
  rb_define_module_function(SpotlightIntern, "get_attribute", method_get_attribute, 2);
}

VALUE cfstring2rbstr(CFStringRef str) {
  CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8);
  char *result = (char *)malloc(sizeof(char) * len);
  CFStringGetCString(str, result, len, kCFStringEncodingUTF8);
  VALUE rb_result = rb_str_new2(result);
  free(result);
  return rb_result;
}

CFStringRef rbstr2cfstring(VALUE str) {
  return CFStringCreateWithCString(kCFAllocatorDefault, StringValuePtr(str), kCFStringEncodingUTF8);
}

CFStringRef date_string(CFDateRef date) {
  CFLocaleRef locale = CFLocaleCopyCurrent();
  CFDateFormatterRef formatter = CFDateFormatterCreate(kCFAllocatorDefault, locale, kCFDateFormatterFullStyle, kCFDateFormatterFullStyle);
  CFStringRef result = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, formatter, date);
  RELEASE_IF_NOT_NULL(formatter);
  RELEASE_IF_NOT_NULL(locale);
  return result;
}

VALUE convert2rb_type(CFTypeRef ref) {
  VALUE result = Qnil;
  double double_result;
  int int_result;
  long long_result;
  int i;
  if (ref != nil) {
    if (CFGetTypeID(ref) == CFStringGetTypeID()) {
      result = cfstring2rbstr(ref);
    } else if (CFGetTypeID(ref) == CFDateGetTypeID()) {
      // 978307200.0 == (January 1, 2001 00:00 GMT) - (January 1, 1970 00:00 UTC)
      // CFAbsoluteTime => January 1, 2001 00:00 GMT
      // ruby Time => January 1, 1970 00:00 UTC
      double_result = (double) CFDateGetAbsoluteTime(ref) + 978307200;
      result = rb_funcall(rb_cTime, rb_intern("at"), 1, rb_float_new(double_result));
    } else if (CFGetTypeID(ref) == CFArrayGetTypeID()) {
      result = rb_ary_new();
      for (i = 0; i < CFArrayGetCount(ref); i++) {
        rb_ary_push(result, convert2rb_type(CFArrayGetValueAtIndex(ref, i)));
      }
    } else if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
      if (CFNumberIsFloatType(ref)) {
        CFNumberGetValue(ref, CFNumberGetType(ref), &double_result);
        result = rb_float_new(double_result);
      } else {
        CFNumberGetValue(ref, CFNumberGetType(ref), &long_result);
        result = LONG2NUM(long_result);
      }
    }
  }
  return result;
}

CFTypeRef convert2cf_type(VALUE obj) {
  CFTypeRef result = nil;
  double double_result;
  int int_result;
  long long_result;
  int i, len;
  VALUE tmp[1];
  CFAbsoluteTime time;

  switch (TYPE(obj)) {
    case T_NIL:
      result = nil;
      break;
    case T_TRUE:
      result = kCFBooleanTrue;
      break;
    case T_FALSE:
      result = kCFBooleanFalse;
      break;
    case T_FLOAT:
      double_result = NUM2DBL(obj);
      result = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &double_result);
      break;
    case T_BIGNUM:
      long_result = NUM2LONG(obj);
      result = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &long_result);
      break;
    case T_FIXNUM:
      int_result = FIX2INT(obj);
      result = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &int_result);
      break;
    case T_STRING:
      result = rbstr2cfstring(obj);
      break;
    case T_DATA:
    case T_OBJECT:
      // todo : check type is Time
      // 978307200.0 == (January 1, 2001 00:00 GMT) - (January 1, 1970 00:00 UTC)
      // CFAbsoluteTime => January 1, 2001 00:00 GMT
      // ruby Time => January 1, 1970 00:00 UTC
      if (rb_obj_is_kind_of(obj, rb_cTime)) {
        time = (CFAbsoluteTime) (NUM2DBL(rb_funcall(obj, rb_intern("to_f"), 0)) - 978307200.0);
        result = CFDateCreate(kCFAllocatorDefault, time);
      }
      break;
    case T_ARRAY:
      len = RARRAY(obj)->len;
      CFTypeRef *values = (CFTypeRef *)malloc(sizeof(CFTypeRef) * len);
      for (i = 0; i < len; i++) {
        tmp[0] = INT2NUM(i);
        values[i] = convert2cf_type(rb_ary_aref(1, tmp, obj));
      }
      result = CFArrayCreate(kCFAllocatorDefault, (const void **)values, len, nil);
      free(values);
      break;
  }
  return result;
}

void set_search_scope(MDQueryRef query, VALUE scopeDirectories) {
  int i;
  int len = RARRAY(scopeDirectories)->len;
  CFStringRef *scopes = (CFStringRef *)malloc(sizeof(CFStringRef) * len);
  for (i=0; i<len; i++) {
    scopes[i] = rbstr2cfstring(rb_ary_pop(scopeDirectories));
  }

  CFArrayRef scopesRef = CFArrayCreate(kCFAllocatorDefault, (const void **)scopes, len, nil);
  free(scopes);

  MDQuerySetSearchScope(query, scopesRef, 0);
  RELEASE_IF_NOT_NULL(scopesRef);
}

VALUE method_search(VALUE self, VALUE queryString, VALUE scopeDirectories) {
  VALUE result = Qnil;
  int i;
  CFStringRef path;
  MDItemRef item;

  CFStringRef qs = rbstr2cfstring(queryString);
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, qs, nil, nil);
  RELEASE_IF_NOT_NULL(qs);
  if (query) {
    set_search_scope(query, scopeDirectories);
    if (MDQueryExecute(query, kMDQuerySynchronous)) {
      result = rb_ary_new();
      for(i = 0; i < MDQueryGetResultCount(query); ++i) {
        item = (MDItemRef) MDQueryGetResultAtIndex(query, i);
        if (item) {
          path = MDItemCopyAttribute(item, kMDItemPath);
          rb_ary_push(result, cfstring2rbstr(path));
          RELEASE_IF_NOT_NULL(path);
        }
      }
    }
    RELEASE_IF_NOT_NULL(query);
  }

  return result;
}

MDItemRef createMDItemByPath(VALUE path) {
  CFStringRef pathRef = rbstr2cfstring(path);
  MDItemRef mdi = MDItemCreate(kCFAllocatorDefault, pathRef);
  RELEASE_IF_NOT_NULL(pathRef);
  return mdi;
}

VALUE method_attributes(VALUE self, VALUE path) {
  int i;
  CFStringRef attrNameRef;
  CFTypeRef attrValueRef;
  MDItemRef mdi = createMDItemByPath(path);

  CFArrayRef attrNamesRef = MDItemCopyAttributeNames(mdi);
  VALUE result = rb_hash_new();
  for (i = 0; i < CFArrayGetCount(attrNamesRef); i++) {
    attrNameRef = CFArrayGetValueAtIndex(attrNamesRef, i);
    attrValueRef = MDItemCopyAttribute(mdi, attrNameRef);
    rb_hash_aset(result, cfstring2rbstr(attrNameRef), convert2rb_type(attrValueRef));
    RELEASE_IF_NOT_NULL(attrValueRef);
  }
  
  RELEASE_IF_NOT_NULL(mdi);
  RELEASE_IF_NOT_NULL(attrNamesRef);

  return result;
}

VALUE method_get_attribute(VALUE self, VALUE path, VALUE name) {
  MDItemRef mdi = createMDItemByPath(path);
  CFStringRef nameRef = rbstr2cfstring(name);
  CFTypeRef valueRef = MDItemCopyAttribute(mdi, nameRef);

  VALUE result = convert2rb_type(valueRef);

  RELEASE_IF_NOT_NULL(valueRef);
  RELEASE_IF_NOT_NULL(nameRef);
  RELEASE_IF_NOT_NULL(mdi);

  return result;
}

VALUE method_set_attribute(VALUE self, VALUE path, VALUE name, VALUE value) {
  MDItemRef mdi = createMDItemByPath(path);
  CFStringRef nameRef = rbstr2cfstring(name);
  CFTypeRef valueRef = convert2cf_type(value);

  MDItemSetAttribute(mdi, nameRef, valueRef);

  RELEASE_IF_NOT_NULL(valueRef);
  RELEASE_IF_NOT_NULL(nameRef);
  RELEASE_IF_NOT_NULL(mdi);

  return Qtrue;
}


