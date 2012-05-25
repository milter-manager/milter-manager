include files.am

files.am: update-files.sh
	update-files

update-files:
	cd $(srcdir); ./update-files.sh > files.am
