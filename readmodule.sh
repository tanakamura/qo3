
tmp=`mktemp -t readmodule.XXXXXX`
rm -f .depend

echo '# Do not edit. This file is generated by readmodule.sh'
echo 'top_dir='`pwd`


for i in `find . -name module.sh # | sed -e 's|^\./||g'`
do
    (mod_dir=`dirname $i`
     . $i
     srcs=`echo $SOURCES | sed -e 's|\([0-9a-zA-Z./_\-][0-9a-zA-Z./_\-]*\)|'${mod_dir}'/\1|g'`
     gen_files=`echo $GENERATED | sed -e 's|\([0-9a-zA-Z./_\-][0-9a-zA-Z./_\-]*\)|'${mod_dir}'/\1|g'`
     redirect_err="# 2>&1 |sed -e 's|${mod_dir}/||g'"
     cat <<EOF
${MODNAME}_SOURCES=${srcs}
${MODNAME}_LEX_SRCS=\$(filter %.l,\$(${MODNAME}_SOURCES))
${MODNAME}_LEX_GEN_SRCS=\$(${MODNAME}_LEX_SRCS:.l=.c)
${MODNAME}_YACC_SRCS=\$(filter %.y,\$(${MODNAME}_SOURCES))
${MODNAME}_YACC_HEADER=\$(${MODNAME}_YACC_SRCS:.y=.h)
${MODNAME}_YACC_GEN_C=\$(${MODNAME}_YACC_SRCS:.y=.c)
\$(${MODNAME}_YACC_GEN_C): \$(${MODNAME}_YACC_SRCS)
\$(${MODNAME}_YACC_HEADER): \$(${MODNAME}_YACC_GEN_C)
${MODNAME}_YACC_GEN_SRCS=\$(${MODNAME}_YACC_GEN_C) \$(${MODNAME}_YACC_HEADER)
${MODNAME}_C_SRCS=\$(filter %.c,\$(${MODNAME}_SOURCES)) \$(${MODNAME}_LEX_GEN_SRCS) \$(${MODNAME}_LEX_SRCS:.l=.flex.h)
${MODNAME}_CXX_SRCS=\$(filter %.cpp,\$(${MODNAME}_SOURCES))
${MODNAME}_ASM_SRCS=\$(filter %.s,\$(${MODNAME}_SOURCES))
${MODNAME}_OBJS=\$(patsubst %.s,%.o,\$(${MODNAME}_ASM_SRCS)) \
    \$(patsubst %.c,%.o,\$(${MODNAME}_C_SRCS)) \$(patsubst %.cpp,%.o,\$(${MODNAME}_CXX_SRCS))
	
${MODNAME}_DIRECTORY=$mod_dir

ifeq (\$(COMPILE_DIR),$mod_dir)
REDIRECT_ERR=${redirect_err}
endif
EOF

     if echo $SOURCES | grep '\.cpp' > /dev/null
     then
	 linker='$(CXX)'
	 
     else
	 linker='$(CC)'
     fi

     module_includes=

     for i in ${INCLUDES}
     do
         module_includes="$includes -I$mod_dir/$i"
     done

     depends=
     includes=$module_includes

     for i in ${NEED_MODULE}
     do
	 depends="$depends \$(${i}_OUTPUT)"
	 includes="$includes -I\$(${i}_DIRECTORY) \$(${i}_INCLUDE)"
     done

     if test "$OUT_EXE" != ""; then
	 cat >>$tmp <<EOF
${mod_dir}/${OUT_EXE}: \$(${MODNAME}_OBJS) ${depends}
	${linker} -o \$@ \$^  \$(ALL_LIBPATH) \$(LDFLAGS) $LDFLAGS \$(REDIRECT_ERR)
EOF
	 cat <<EOF
${MODNAME}_OUTPUT=${mod_dir}/${OUT_EXE}
EOF


	 target="${mod_dir}/${OUT_EXE}"
     elif test "$OUT_LIB" != ""; then
	 cat >>$tmp <<EOF
${mod_dir}/${OUT_LIB}: \$(${MODNAME}_OBJS)  ${depends}
	ar rsu \$@ \$^
EOF
	 cat <<EOF
ALL_LIBPATH+=-L${mod_dir}
${MODNAME}_OUTPUT=${mod_dir}/${OUT_LIB}
EOF
	 target="${mod_dir}/${OUT_LIB}"
     fi

     if echo $SOURCES | grep '.l'>/dev/null ; then
	 LDFLAGS="$LDFLAGS -lfl"
     fi
     cat<<EOF

${MODNAME}_INCLUDE=$includes

${mod_dir}/%.o: ${mod_dir}/%.c
	\$(CC) -o \$@ -c -I${mod_dir} \$(CFLAGS) \$(CPPFLAGS) \$(${MODNAME}_INCLUDE) $CFLAGS $CPPFLAGS \$< \$(REDIRECT_ERR)
${mod_dir}/%.o: ${mod_dir}/%.cpp
	\$(CXX) -o \$@ -c -I${mod_dir} \$(CXXFLAGS) \$(CPPFLAGS) \$(${MODNAME}_INCLUDE) $CXXFLAGS $CPPFLAGS \$< \$(REDIRECT_ERR)
${mod_dir}/%.o: ${mod_dir}/%.s
	\$(AS) -o \$@ \$(ASFLAGS) \$<
${mod_dir}/%.c: ${mod_dir}/%.l
	flex --header-file=\$(patsubst %.l,%.flex.h,\$(${MODNAME}_LEX_SRCS)) --outfile=\$@ \$< \$(REDIRECT_ERR)
${mod_dir}/%.c: ${mod_dir}/%.y
	yacc -d $<
	mv -f y.tab.c \$@
	mv -f y.tab.h ${mod_dir}/\$*.h
${mod_dir}/%.h: ${mod_dir}/%.y
	yacc -d $<
	mv -f y.tab.c \$@
	mv -f y.tab.h ${mod_dir}/\$*.h

${MODNAME}_GEN_SOURCES=\$(${MODNAME}_LEX_GEN_SRCS) \$(${MODNAME}_YACC_GEN_SRCS) ${gen_files}
${MODNAME}_GEN_FILES=$target \$(${MODNAME}_LEX_GEN_SRCS) \$(${MODNAME}_OBJS) \$(${MODNAME}_GEN_SOURCES)

ALL_GEN_FILES+=\$(${MODNAME}_GEN_FILES) ${mod_dir}/Makefile
ALL_SOURCES+=\$(${MODNAME}_C_SRCS) \$(${MODNAME}_CXX_SRCS)
ALL_TARGETS+=$target
ALL_GEN_SOURCES+=\$(${MODNAME}_GEN_SOURCES)
DEPEND_INC+=-I${mod_dir} ${module_includes}

${MODNAME}_clean:
	rm -f \$(${MODNAME}_GEN_FILES)

${POST_APPEND}

EOF
     cat >$mod_dir/Makefile <<EOF
all:
	make -C $PWD $target COMPILE_DIR="${mod_dir}"
clean:
	make -C $PWD ${MODNAME}_clean COMPILE_DIR="${mod_dir}"
redep:
	make -C $PWD redep COMPILE_DIR="${mod_dir}"
EOF

    )
done

cat $tmp

rm $tmp
