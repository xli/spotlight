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
  free(result);
  return rb_str_new2(result);
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
      result = CFArrayCreate(kCFAllocatorDefault, values, len, nil);
      free(values);
      break;
  }
  return result;
}

VALUE method_search(VALUE self, VALUE queryString, VALUE scopeDirectory) {
  
  CFStringRef qs = rbstr2cfstring(queryString);
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, qs, nil, nil);
  VALUE result = Qnil;
  int i;

  if (query) {
    CFStringRef scopeDirectoryStr = rbstr2cfstring(scopeDirectory);
    CFStringRef scopeDirectoryStrs[1] = {scopeDirectoryStr};
    RELEASE_IF_NOT_NULL(scopeDirectoryStr);

    CFArrayRef scopeDirectories = CFArrayCreate(kCFAllocatorDefault, (const void **)scopeDirectoryStrs, 1, nil);
    MDQuerySetSearchScope(query, scopeDirectories, 0);
    RELEASE_IF_NOT_NULL(scopeDirectories);

    if (MDQueryExecute(query, kMDQuerySynchronous)) {
      result = rb_ary_new();
      for(i = 0; i < MDQueryGetResultCount(query); ++i) {
        MDItemRef item = (MDItemRef) MDQueryGetResultAtIndex(query, i);
        CFStringRef path = MDItemCopyAttribute(item, kMDItemPath);
        rb_ary_push(result, cfstring2rbstr(path));

        RELEASE_IF_NOT_NULL(path);
      }
    }
    RELEASE_IF_NOT_NULL(query);
  }

  RELEASE_IF_NOT_NULL(qs);
  return result;
}

VALUE method_attributes(VALUE self, VALUE path) {
  VALUE result = rb_hash_new();
  MDItemRef mdi = MDItemCreate(kCFAllocatorDefault, rbstr2cfstring(path));
  CFArrayRef attr_names = MDItemCopyAttributeNames(mdi);
  int i;
  for (i = 0; i < CFArrayGetCount(attr_names); i++) {
    CFStringRef attr_name = CFArrayGetValueAtIndex(attr_names, i);
    CFTypeRef attr_value = MDItemCopyAttribute(mdi, attr_name);
    rb_hash_aset(result, cfstring2rbstr(attr_name), convert2rb_type(attr_value));
    RELEASE_IF_NOT_NULL(attr_value);
  }
  
  RELEASE_IF_NOT_NULL(mdi);
  RELEASE_IF_NOT_NULL(attr_names);

  return result;
}

VALUE method_get_attribute(VALUE self, VALUE path, VALUE name) {
  MDItemRef mdi = MDItemCreate(kCFAllocatorDefault, rbstr2cfstring(path));
  CFStringRef attr_name = rbstr2cfstring(name);
  CFTypeRef attr_value = MDItemCopyAttribute(mdi, attr_name);
  VALUE result = convert2rb_type(attr_value);

  RELEASE_IF_NOT_NULL(attr_value);
  RELEASE_IF_NOT_NULL(mdi);

  return result;
}

VALUE method_set_attribute(VALUE self, VALUE path, VALUE name, VALUE value) {
  MDItemRef item = MDItemCreate(kCFAllocatorDefault, rbstr2cfstring(path));
  CFStringRef cfName = rbstr2cfstring(name);
  CFTypeRef cfValue = convert2cf_type(value);
  MDItemSetAttribute(item, cfName, cfValue);

  RELEASE_IF_NOT_NULL(cfValue);
  RELEASE_IF_NOT_NULL(cfName);
  RELEASE_IF_NOT_NULL(item);

  return Qtrue;
}


