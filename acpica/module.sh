MODNAME=acpica
OUT_LIB=libacpica.a
NEED_MODULE="libc"
dirs="utilities events namespace executer dispatcher hardware parser tables resources disassembler"
SOURCES=""
for i in $dirs
do
    files=$mod_dir/$i/*.c
    for j in $files
    do
        j=`basename $j`
        SOURCES="$SOURCES $i/$j"
    done
done

INCLUDES=include

