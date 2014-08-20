export OPENWRTDIR=~/sk-probe/
export STAGING_DIR=$OPENWRTDIR/trunk/staging_dir/
#PATH=$STAGING_DIR/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33/usr/bin:$PATH 
#PATH=$STAGING_DIR/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33/usr/lib:$PATH 
PATH=$STAGING_DIR/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33/bin:$PATH 
mips-openwrt-linux-uclibc-g++ \
-I$STAGING_DIR/target-mips_r2_uClibc-0.9.33/usr/include/ \
-L$STAGING_DIR/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33/lib \
-L$STAGING_DIR/target-mips_r2_uClibc-0.9.33/root-ar71xx/usr/lib \
src/*.cpp src/*.h -o youtubetest-rel -Wl,-rpath-link=$STAGING_DIR/target-mips_r2_uClibc-0.9.33/root-ar71xx/usr/lib $STAGING_DIR/target-mips_r2_uClibc-0.9.33/root-ar71xx/usr/lib/libcurl.so.4 
