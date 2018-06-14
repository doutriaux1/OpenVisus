#!/bin/bash

rm -Rf  temp/*

VISUS=$1
DATASETS=$2

if [ ! $VISUS ]; then
	VISUS="build/Debug/visus.exe"
fi

if [ ! $DATASETS ]; then
	DATASETS="../../datasets"
	if [ ! -d $DATASETS ] ; then
	  DATASETS="./datasets"
	fi
fi

# ///////////////////////////////////////////
# For very simple conversions, all you need is create
# ///////////////////////////////////////////
$VISUS import $DATASETS/cat/gray.tga create temp/cat/gray.idx 
$VISUS import $DATASETS/cat/rgb.tga  create temp/cat/rgb.idx 


# ///////////////////////////////////////////
# To get information about any file (including non-idx), use info
# ///////////////////////////////////////////
$VISUS info $DATASETS/cat/gray.tga
$VISUS info $DATASETS/cat/rgb.tga
$VISUS info temp/cat/gray.idx
$VISUS info temp/cat/rgb.idx


# ///////////////////////////////////////////
# For more control, explicitly create the idx volume
# ///////////////////////////////////////////
fields="scalar uint8 compressed + vector uint8[3] compressed"

$VISUS create temp/test_formats.idx --box "0 511 0 511" --fields "$fields" --bitsperblock 12

# ///////////////////////////////////////////
# example of tiling the same image multiple times
# ///////////////////////////////////////////

$VISUS import $DATASETS/cat/gray.tga export temp/test_formats.idx --field scalar --box "  0 255     0 255"
$VISUS import $DATASETS/cat/gray.tga export temp/test_formats.idx --field scalar --box "256 511     0 255"
$VISUS import $DATASETS/cat/gray.tga export temp/test_formats.idx --field scalar --box "  0 255   256 511"
$VISUS import $DATASETS/cat/gray.tga export temp/test_formats.idx --field scalar --box "256 511   256 511"

$VISUS import $DATASETS/cat/rgb.tga export temp/test_formats.idx --field vector --box "  0 255     0 255"
$VISUS import $DATASETS/cat/rgb.tga export temp/test_formats.idx --field vector --box "256 511     0 255"
$VISUS import $DATASETS/cat/rgb.tga export temp/test_formats.idx --field vector --box "  0 255   256 511"
$VISUS import $DATASETS/cat/rgb.tga export temp/test_formats.idx --field vector --box "256 511   256 511"


$VISUS import temp/test_formats.idx --field scalar --box "  0 255     0 255"  export temp/test_formats_scalar.tga 
$VISUS import temp/test_formats.idx --field scalar --box "256 511     0 255"  export temp/test_formats_scalar.jpg  
$VISUS import temp/test_formats.idx --field scalar --box "  0 255   256 511"  export temp/test_formats_scalar.tif  
$VISUS import temp/test_formats.idx --field scalar --box "256 511   256 511"  export temp/test_formats_scalar.bmp 
$VISUS import temp/test_formats.idx --field scalar --box "128 383   128 383"  export temp/test_formats_scalar.png 


$VISUS import temp/test_formats.idx --field vector --box "  0 255     0 255"  export temp/test_formats_vector.tga 
$VISUS import temp/test_formats.idx --field vector --box "256 511     0 255"  export temp/test_formats_vector.jpg  
$VISUS import temp/test_formats.idx --field vector --box "  0 255   256 511"  export temp/test_formats_vector.tif  
$VISUS import temp/test_formats.idx --field vector --box "256 511   256 511"  export temp/test_formats_vector.bmp 
$VISUS import temp/test_formats.idx --field vector --box "128 383   128 383"  export temp/test_formats_vector.png 

# ///////////////////////////////////////////
# example of inplace pasting an image
# ///////////////////////////////////////////

$VISUS create temp/test_paste.idx --box "0 255 0 1023" --fields "DATA uint8[3] default_compression(zip)" --bitsperblock 12

# show how several files can be "merged" together in import

$VISUS \
	import /dev/null --dtype 3*uint8 --dims "256 1024" \
	paste $DATASETS/cat/rgb.tga --destination-box "0 255 0   255"  \
	paste $DATASETS/cat/rgb.tga --destination-box "0 255 256 511"  \
	paste $DATASETS/cat/rgb.tga --destination-box "0 255 512 767"  \
	paste $DATASETS/cat/rgb.tga --destination-box "0 255 768 1023" \
	export temp/test_paste.idx --box "0 255 0 1023"
	
$VISUS \
  import temp/test_paste.idx --box "0 255 0 1023" \
  export temp/test_paste.jpg

# ///////////////////////////////////////////
# example of several timesteps
# ///////////////////////////////////////////

$VISUS create temp/test_timesteps.idx --box "0 255 0 255" --fields "scalar uint8 default_compression(zip)" --time 0 3 time%02d/

$VISUS import $DATASETS/cat/gray.tga export temp/test_timesteps.idx --time 0 --box "0 255 0 255"
$VISUS import $DATASETS/cat/gray.tga export temp/test_timesteps.idx --time 1 --box "0 255 0 255"
$VISUS import $DATASETS/cat/gray.tga export temp/test_timesteps.idx --time 2 --box "0 255 0 255"
$VISUS import $DATASETS/cat/gray.tga export temp/test_timesteps.idx --time 3 --box "0 255 0 255"

$VISUS import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 0 export temp/test_timesteps_0.tga 
$VISUS import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 1 export temp/test_timesteps_1.jpg  
$VISUS import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 2 export temp/test_timesteps_2.tif  
$VISUS import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 3 export temp/test_timesteps_3.bmp 

# ///////////////////////////////////////////
# example of getting input from network (works using idx too!)
# ///////////////////////////////////////////

$VISUS \
  import "http://www.dia.uniroma3.it/~scorzell/phd/docs/html/fig/tower.all.gif" \
  export temp/test_network_0.png
  
$VISUS \
  import "http://atlantis.sci.utah.edu/mod_visus?dataset=atlanta&compression=jpg" --box "16000 17023 14000 15034" \
  export temp/test_network_1.png



