cvt() {
	rm $1.html.gz $1.h $1.gz.h 2> /dev/null
	gzip -k $1.html
	xxd -n $1_html -i $1.html $1.h
	xxd -n $1_html_gz -i $1.html.gz $1.gz.h
}
cvt "filemgr"
cvt "landing"
cvt "wifi"
