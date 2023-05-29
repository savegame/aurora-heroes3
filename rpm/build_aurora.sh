#!/bin/bash

_destdir=$1
_app_name=$2

strip bin/%{name}
strip bin/libvcmi.so
for each in bin/AI/*; do strip $each; done

make DESTDIR=${_destdir} install

# remove libtbb stuff
rm -fr ${_destdir}/usr/include
rm -fr ${_destdir}/usr/share/doc
rm -fr ${_destdir}/usr/lib

# copy boost libraries from buildengine
names="libboost_atomic-mt.so.1
libboost_chrono-mt.so.1
libboost_date_time-mt.so.1
libboost_filesystem-mt.so.1
libboost_locale-mt.so.1
libboost_program_options-mt.so.1
libboost_system-mt.so.1
libboost_thread-mt.so.1
libboost_atomic-mt.so.1
libboost_chrono-mt.so.1
libboost_date_time-mt.so.1
libboost_filesystem-mt.so.1
libboost_locale-mt.so.1
libboost_program_options-mt.so.1
libboost_system-mt.so.1
libboost_thread-mt.so.1
libicui18n.so.
libicuuc.so."

for each in $names; do 
    name=`ls /usr/lib/|grep $each`
    for _name in $name; do
        rsync -avP "/usr/lib/$_name"  ${_destdir}/usr/share/${_app_name}/lib/
    done
done