name:=bfci

compile:
	gcc main.c -no-pie -o $(name)

install: $(outfile)
	mv $(name) /usr/local/bin
