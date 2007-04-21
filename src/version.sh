#! /bin/sh

# Shell script to generate the version header.

PATH=/bin:/usr/bin

build_num_file="../buildnum.data"

# Update build number.
if test -f "${build_num_file}"; then
	build_num=`awk '{ print $1 + 1 }' < "${build_num_file}"`
else
	build_num="1"
fi

echo "${build_num}" > "${build_num_file}"

echo "Creating version.h (build ${build_num})"

# Write version.h header.
cat <<EOF > version.h
#ifndef BT_VERSION_H
#define BT_VERSION_H

EOF

# Write definitions.
echo "#define SVN_REVISION \"`svnversion ..`\"" >> version.h
echo "#define MUX_BUILD_DATE \"`date`\"" >> version.h
echo "#define MUX_BUILD_NUM \"${build_num}\"" >> version.h
echo >> version.h
echo "#define MUX_BUILD_USER \"`whoami`\"" >> version.h
echo "#define MUX_BUILD_HOST \"`hostname -f`\"" >> version.h

# Write version.h trailer.
cat <<EOF >> version.h

#endif /* !BT_VERSION_H */
EOF
