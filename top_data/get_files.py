#! /usr/bin/python

from urllib import urlretrieve

# Note: The version has changed in the past. Make sure it's up to date
prefix = 'http://dds.cr.usgs.gov/srtm/version2_1/SRTM3/North_America/'

for lat in range(30, 50):
	for lon in range(110, 130):
		print 'Downloading file "N%dW%d.hgt.zip"' % (lat, lon)
		urlretrieve(prefix + 'N%dW%d.hgt.zip' % (lat, lon), 'N%dW%d.hgt.zip' % (lat, lon))
