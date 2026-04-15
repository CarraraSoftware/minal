#!/usr/bin/zsh

# prints the hexadecimal and text representations
# for every ANSI escape sequence supported by
# the terminal passed as the first positional argument
# or the one defined in $TERM if no argument is passed.
#
# ./termescapes "st-256color"
# // prints escapes for st terminal
#
# ./termescapes
# // prints escapes for whatever is on $TERM


getcodes() {
    local t
    if [ $# -lt 1 ] ; then
        t=$TERM;
    else
        t=$1;
    fi
    codes=$(infocmp -1 -T $t| tail -n +3 | sed -E 's/\s*([^=^#]*).*,$/\1/g')
    echo $codes
}

printcodes() {
    local codes=$1
    for code (${(f)codes}); do
        echo "$code: "
        builtin echoti $code | xxd 
        echo "---"
    done
}

main() {
    local codes=$(getcodes)
    printcodes $codes
}

getcodes $1
# main $1
