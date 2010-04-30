require 'mkmf'

$LDFLAGS << " -framework CoreServices"
have_header("CoreServices/CoreServices.h")

create_makefile('spotlight/spotlight')