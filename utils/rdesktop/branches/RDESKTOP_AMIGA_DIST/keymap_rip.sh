#!/bin/sh

echo "FIXME: code has changed, must fix this script." exit if[-z "$1"] ;
then echo Usage:$0 p
    ath_to_wine / windows / x11drv / keyboard.c
    exit
    fi
    out = keymap.h
    if[-f $out] ; then
    echo File \ "$out\" exists. Bailing out.
    exit
    fi
    sed - n - e "1,/\*\//p" \
    -e "/#define MAIN_LEN/,/}/p" \
    -e "/char main_key_US/,/Layout table/p" "$@" > $out
#echo "struct { char* id; char** layout } static const KeybLayout {" >>$out
# why has microsoft it's own ways?
#echo "{\"GR\",main_key_DE}," >>$out
#sed -n "s/^.*char main_key_\(..\)\[.*$/{\"\1\",main_key_\1},/p" <$out >$out.tmp
#cat $out.tmp >>$out
#echo "{NULL,NULL}};" >>$out
#rm $out.tmp
