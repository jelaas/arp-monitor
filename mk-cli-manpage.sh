#!/bin/bash

#
# mk-manpage.sh CATEGORY SECTIONNO COPYRIGHT
# mk-manpage.sh "My category" 1 "John Doe, Anomymous Inc"
#

export DATE=$(date "+%B %Y")

CATEGORY="${1:-Your own category}"
export SECTION="${2:-1}"
export COPYRIGHT="${3:-John Doe, Anomymous Inc}"

function convertU {
    local LINE
    unset IFS
    read LINE
    if [ "$CONVERTS" ]; then
#	echo "$LINE" >&2
	for CON in $CONVERTS; do
	    SCON="${CON:1:${#CON}-2}"
	    LINE=$(echo "$LINE"|sed "s/$CON/\\\fI${SCON}\\\fP/g")
	done
    fi
    echo "$LINE"|sed "s/\\ESC/\\x5c\\x5c\\x5c\\x5c/g"
}

function convertUO {
    read LINE
    if [ "$CONVERTS" ]; then
	for CON in $CONVERTS; do
	    SCON="${CON:1:${#CON}-2}"
	    LINE=$(echo "$LINE"|sed "s/$CON/\\\fI${SCON}\\\fP/g")
	done
    fi
    for w in $LINE EOLXX; do
	if [ "$w" = EOLXX ]; then
	    break;
	fi
	if [ "${w:0:1}" = "-" ]; then
#	    echo "::$w::" >&2
	    if echo "$w"|grep -q = ; then
		wf=$(echo "$w"|cut -d = -f 1)
		ws=$(echo "$w"|cut -d = -f 2)
		echo -n "\\fB$wf\\fR=$ws\\fR "
	    else
		echo -n "\\fB$w\\fR "
	    fi
	else
	    echo -n "$w "
	fi
    done
    echo "\\fR"
#    echo "$LINE"|sed "s/\\ESC/\\x5c\\x5c\\x5c\\x5c/g"
}

function convertUs {
    read LINE
    if [ "$CONVERTS" ]; then
	for CON in $CONVERTS; do
	    SCON="${CON:1:${#CON}-2}"
	    LINE=$(echo "$LINE"|sed "s/*$SCON,/*\\\fI${SCON}\\\fP,/g")
	    LINE=$(echo "$LINE"|sed "s/*$SCON)/*\\\fI${SCON}\\\fP)/g")
	    LINE=$(echo "$LINE"|sed "s/ $SCON,/ \\\fI${SCON}\\\fP,/g")
	    LINE=$(echo "$LINE"|sed "s/ $SCON)/ \\\fI${SCON}\\\fP)/g")
	done
	echo "$LINE"
    else
	echo "$LINE"
    fi
}

function manpage {
    read NAME
    read CONVERTS
    if [ "$CONVERTS" ]; then
	read
    fi
    
    echo ".TH \"$NAME\" \"$SECTION\" \"$DATE\" \"$CATEGORY\""
    echo ".SH NAME"
    echo "$NAME"
    echo ".SH SYNOPSIS"
    while read LINE; do
	echo ".BI \"$LINE\""|convertU
	if [ -z "$LINE" ]; then
	    break;
	fi
    done
    echo ".sp"
#    echo ".B $(egrep "[ *]$NAME[(]" $HEADERFILE|head -1)"|convertUs
    echo ".SH DESCRIPTION"
    IFS=""
    while read LINE; do
	if [ "${LINE:0:2}" = "  " ]; then
	    echo ".HP"
	    echo "\\\fB${LINE:2}\\\fR"|(convertU)
	    echo ".br"
	    continue
	fi
	echo "$LINE"|(convertU)
	if [ -z "$LINE" ]; then
	    break;
	fi
	echo ".br"
    done
    echo ".SH FILES" >tmppage-bh
    while read LINE; do
	if [ "${LINE:0:2}" = "  " ]; then
	    echo ".HP" >>tmppage-bh
	    echo "\\\fB${LINE:2}\\\fR"|(convertU) >>tmppage-bh
	    echo ".br" >>tmppage-bh
	    continue
	fi
	echo "$LINE"|(convertU) >>tmppage-bh
	if [ -z "$LINE" ]; then
	    break;
	fi
	echo ".br" >>tmppage-bh
    done
    unset IFS
    
    echo ".SH COPYRIGHT" >>tmppage-bh
    echo "Copyright \(co $(date +%Y) $COPYRIGHT." >>tmppage-bh
    echo "This is free software; see the source for copying conditions.  There is NO" >>tmppage-bh
    echo "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE," >>tmppage-bh
    echo "to the extent permitted by law." >>tmppage-bh
    echo ".SH SEE ALSO" >>tmppage-bh
    while read LINE; do
	echo ".BR $LINE"|convertU >>tmppage-bh
	if [ -z "$LINE" ]; then
	    break;
	fi
    done

    echo ".SH OPTIONS"
    while read LINE; do
	echo ".HP"
	echo "$LINE"|convertUO
	if [ -z "$LINE" ]; then
	    break;
	fi
	read LINE
	echo ".IP"
	echo "$LINE"|convertU
    done
    cat tmppage-bh
    rm -f tmppage-bh
}

mkdir -p man
for f in *.mn; do
    M=$(basename $f|cut -d . -f 1).$SECTION
    echo "Generating $M from $f"
    cat $f | manpage >man/$M
done

# Sections: NAME, SYNOPSIS, DESCRIPTION, OPTIONS, FILES, EXAMPLES, COPYRIGHT, SEE ALSO