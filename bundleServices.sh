
Version="0.11.0"

if [ -z "$1" ]
then
   filename="SolARService_Relocalization_$Version"
else
   filename=$1
fi

# TODO: get rid of hack that removes cuda configuration
# This hack is introduced because we want only one version in the bundle otherwise
# conflicting cuda libraries (e.g. libopencv_dnn.so) will conflict and overwrite
# its non-cuda counterpart.
CONFIG_FILES=`find . -not \( -path "./deploy/*" -prune \) -type f \( -name "SolARService*_conf.xml" -or -name "SolARService*_modules*.xml" \) | grep -vi cuda`

echo "**** Install dependencies locally"
remaken install packagedependencies.txt
remaken install packagedependencies.txt -c debug

echo "**** Bundle dependencies in bin folder"
for file in $CONFIG_FILES
do
   echo "install dependencies for config file: $file"
   remaken bundleXpcf --recurse -d ./deploy/bin/x86_64/shared/release -s modules $file
   remaken bundleXpcf --recurse -d ./deploy/bin/x86_64/shared/debug -s modules -c debug $file
done

# TODO: remove cuda hack (same as above)
find ./deploy/bin/x86_64/shared -iname "*cuda*" -exec rm {} \;

zip --symlinks -r "./deploy/${filename}_release.zip" ./deploy/bin/x86_64/shared/release ./README.md ./installData.sh ./LICENSE
zip --symlinks -r "./deploy/${filename}_debug.zip" ./deploy/bin/x86_64/shared/debug ./README.md ./installData.sh ./LICENSE
