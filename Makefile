

all: main

main: main.c rtfslconst.c rtfsldata.c rtfsldelete.c rtfslfailsafe.c rtfslfileseek.c rtfslfilestat.c rtfslfiliocore.c rtfslfiliord.c rtfslfiliowr.c rtfslfssystem.c rtfslgfirst.c rtfslitecore.c rtfsliteshell.c rtfslmkdir.c rtfslopenpath.c rtfslrename.c rtfslrmdir.c rtfslsystem.c rtfstlitefileload.c rtfstlitetestfileio.c rtfstlitetests.c rtfstlitetestutils.c
	gcc -g -o main main.c rtfslconst.c rtfsldata.c rtfsldelete.c rtfslfailsafe.c rtfslfileseek.c rtfslfilestat.c rtfslfiliocore.c rtfslfiliord.c rtfslfiliowr.c rtfslfssystem.c rtfslgfirst.c rtfslitecore.c rtfsliteshell.c rtfslmkdir.c rtfslopenpath.c rtfslrename.c rtfslrmdir.c rtfslsystem.c rtfstlitefileload.c rtfstlitetestfileio.c rtfstlitetests.c rtfstlitetestutils.c
