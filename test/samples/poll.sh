echo hello
while [[ $(cat key_file.txt) != "content" ]]; do
  sleep 0.05
done

echo world
