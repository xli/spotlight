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

#ifndef RUBY_19
#ifndef RARRAY_LEN
#define RARRAY_LEN(v) (RARRAY(v)->len)
#endif
#endif

Boolean MDItemSetAttribute(MDItemRef item, CFStringRef name, CFTypeRef value);

static VALUE cfstring2rbstr(CFStringRef str) {
  const char *result;
  VALUE rb_result = Qnil;
  CFDataRef data = CFStringCreateExternalRepresentation(kCFAllocatorDefault, str, kCFStringEncodingUTF8, 0);
  if (data) {
    result = (const char *)CFDataGetBytePtr(data);
    rb_result = rb_str_new2(result);
  }
  RELEASE_IF_NOT_NULL(data)
  return rb_result;
}

static CFStringRef rbstr2cfstring(VALUE str) {
  return CFStringCreateWithCString(kCFAllocatorDefault, StringValuePtr(str), kCFStringEncodingUTF8);
}

static CFStringRef date_string(CFDateRef date) {
  CFLocaleRef locale = CFLocaleCopyCurrent();
  CFDateFormatterRef formatter = CFDateFormatterCreate(kCFAllocatorDefault, locale, kCFDateFormatterFullStyle, kCFDateFormatterFullStyle);
  CFStringRef result = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, formatter, date);
  RELEASE_IF_NOT_NULL(formatter);
  RELEASE_IF_NOT_NULL(locale);
  return result;
}

static VALUE convert2rb_type(CFTypeRef ref) {
  VALUE result = Qnil;
  double double_result;
  int int_result;
  long long_result;
  int i;
  if (ref) {
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

static CFTypeRef convert2cf_type(VALUE obj) {
  CFTypeRef result = NULL;
  double double_result;
  int int_result;
  long long_result;
  int i, len;
  VALUE tmp[1];
  CFAbsoluteTime time;
  CFMutableArrayRef array_result;

  switch (TYPE(obj)) {
    case T_NIL:
      result = NULL;
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
      len = RARRAY_LEN(obj);
      array_result = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
      for (i = 0; i < len; i++) {
        tmp[0] = INT2NUM(i);
        CFArrayAppendValue(array_result, convert2cf_type(rb_ary_aref(1, tmp, obj)));
      }
      result = array_result;
      break;
    default:
      rb_raise(rb_eTypeError, "not valid value");
      break;
  }
  return result;
}

static void set_search_scope(MDQueryRef query, VALUE scopeDirectories) {
  int i;
  int len = RARRAY_LEN(scopeDirectories);
  CFMutableArrayRef scopesRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
  for (i=0; i<len; i++) {
    CFArrayAppendValue(scopesRef, rbstr2cfstring(rb_ary_pop(scopeDirectories)));
  }

  MDQuerySetSearchScope(query, scopesRef, 0);
  RELEASE_IF_NOT_NULL(scopesRef);
}

VALUE method_search(VALUE self, VALUE queryString, VALUE scopeDirectories) {
  VALUE result = Qnil;
  int i;
  CFStringRef path;
  MDItemRef item;

  CFStringRef qs = rbstr2cfstring(queryString);
  MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, qs, NULL, NULL);
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

static MDItemRef createMDItemByPath(VALUE path) {
  CFStringRef pathRef = rbstr2cfstring(path);
  MDItemRef mdi = MDItemCreate(kCFAllocatorDefault, pathRef);
  RELEASE_IF_NOT_NULL(pathRef);
  if (!mdi) {
    rb_raise(rb_eTypeError, "Could not find MDItem by given path");
  }
  return mdi;
}

static int each_attribute(VALUE name, VALUE value, MDItemRef mdi) {
  CFStringRef nameRef = rbstr2cfstring(name);
  CFTypeRef valueRef = convert2cf_type(value);

  if(!MDItemSetAttribute(mdi, nameRef, valueRef)) {
    printf("set %s failed\n", StringValuePtr(name));
  }

  RELEASE_IF_NOT_NULL(valueRef);
  RELEASE_IF_NOT_NULL(nameRef);

  return ST_CONTINUE;
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

  if(!MDItemSetAttribute(mdi, nameRef, valueRef)) {
    printf("set %s failed\n", StringValuePtr(name));
  }

  RELEASE_IF_NOT_NULL(valueRef);
  RELEASE_IF_NOT_NULL(nameRef);
  RELEASE_IF_NOT_NULL(mdi);

  return Qtrue;
}

VALUE method_set_attributes(VALUE self, VALUE path, VALUE attributes) {
  MDItemRef mdi = createMDItemByPath(path);
  rb_hash_foreach(attributes, each_attribute, (VALUE) mdi);
  RELEASE_IF_NOT_NULL(mdi);

  return Qnil;
}

void Init_spotlight (void)
{
  VALUE Spotlight = rb_define_module("Spotlight");
  VALUE SpotlightIntern = rb_define_module_under(Spotlight, "Intern");
  rb_define_module_function(SpotlightIntern, "search", method_search, 2);
  rb_define_module_function(SpotlightIntern, "attributes", method_attributes, 1);
  rb_define_module_function(SpotlightIntern, "set_attribute", method_set_attribute, 3);
  rb_define_module_function(SpotlightIntern, "get_attribute", method_get_attribute, 2);
  rb_define_module_function(SpotlightIntern, "set_attributes", method_set_attributes, 2);
}
