#! /bin/sh

# erase SVN metadata
find . -wholename "*/.svn/*" -delete
find . -wholename "*/.svn" -delete

#find . -wholename "*/bak.svn/*" -delete
#find . -wholename "*/bak.svn" -delete

# erase temporary files (Edyuk convention : build files but not bins)
find . -wholename "*/tmp-*/*" -delete
find . -wholename "*/tmp-*" -delete
find . -wholename "*/Makefile" -delete
find . -wholename "*/Makefile.*" -delete