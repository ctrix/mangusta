#!/bin/bash

INDENT=`which gindent`

if [ "x$INDENT" == "x" ]; then
    INDENT=`which indent`

    if [ "x$INDENT" == "x" ]; then
        INDENT="indent"
    fi
fi

function do_indent() {
    local FROM=$1
    local TO=$2

    BN=`basename $FROM`
    if [ "x$BN" == "xliblit_3d_circlegrid.cpp" ]; then
	return;
    fi


    $INDENT \
        --braces-on-func-def-line \
        --no-blank-lines-after-declarations \
        \
        --no-blank-lines-after-procedures \
        --no-blank-lines-after-commas \
        --break-before-boolean-operator \
        --braces-on-struct-decl-line \
        --honour-newlines \
        --braces-on-if-line \
        --comment-indentation33 \
        --declaration-comment-column33 \
        --no-comment-delimiters-on-blank-lines \
        --cuddle-else \
        --continuation-indentation4 \
        --case-indentation0 \
        --line-comments-indentation0 \
        --declaration-indentation1 \
        --dont-format-first-column-comments \
        --parameter-indentation0 \
        --continue-at-parentheses \
        --no-space-after-function-call-names \
        --dont-break-procedure-type \
        --space-after-if \
        --space-after-for \
        --space-after-while \
        --no-space-after-parentheses \
        --dont-star-comments \
        --swallow-optional-blank-lines \
        --dont-format-comments \
        --else-endif-column33 \
        --space-special-semicolon \
        --indent-label1 \
	--case-indentation4 \
        \
        --tab-size4 \
        -i4 \
        -l200 \
        --no-tabs \
        $FROM \
        -o $TO
}

SOURCES=`find src/ -name *.c -print;`
for SOURCE in $SOURCES; do
    do_indent $SOURCE $SOURCE
done;

SOURCES=`find src/ -name *.cpp -print;`
for SOURCE in $SOURCES; do
    do_indent $SOURCE $SOURCE
done;

SOURCES=`find src/ -name *.h -print;`
for SOURCE in $SOURCES; do
    do_indent $SOURCE $SOURCE
done;

exit;
