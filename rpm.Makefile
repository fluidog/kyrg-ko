rpm-pkg:
	rpmbuild --noprep --build-in-place --define="_topdir $$PWD/rpmbuild" -bb kyrg.spec
tar-pkg:
	tar -czf kyrg.tar.gz --exclude='rpmbuild/*' *
