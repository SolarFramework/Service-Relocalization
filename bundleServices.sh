
Version="1.0.0"

if [ -z "$1" ]
then
   filename="SolARService_Relocalization_$Version"
else
   filename=$1
fi

CONFIG_FILES=`find . -not \( -path "./deploy/bin" -prune \) -type f \( -name "SolARService*_conf.xml" -or -name "SolARServiceTest*_conf.xml" \)`

echo "**** Install dependencies locally"
remaken install packagedependencies.txt
remaken install packagedependencies.txt -c debug

echo "**** Bundle dependencies in bin folder"
for file in $CONFIG_FILES
do
   echo "install dependencies for config file: $file"
   remaken bundleXpcf --recurse -d ./deploy/x86_64/shared/release -s modules $file
   remaken bundleXpcf --recurse -d ./deploy/x86_64/shared/debug -s modules -c debug $file
done

zip --symlinks -r "./deploy/${filename}_release.zip" ./deploy/x86_64/shared/release ./README.md ./installData.sh ./LICENSE
zip --symlinks -r "./deploy/${filename}_debug.zip" ./deploy/x86_64/shared/debug ./README.md ./installData.sh ./LICENSE 
