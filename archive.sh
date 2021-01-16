project_name=cscheme

[[ -z "$1" ]] && {
  echo "Please pass version as argument"
  exit 1
}

path=./build/$project_name-$1
git archive --prefix $project_name/ --output $path.tar --format tar HEAD &&
git archive --prefix $project_name/ --output $path.tar.gz --format tar.gz HEAD && {
  echo "Archive generated to $path"
  exit 0
}
