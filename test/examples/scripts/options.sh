close="save lemonbar.options.raw.mode=close"
hibernate="A:systemctl hibernate:}hibernate%{A"
open="A:save lemonbar.options.raw.mode=open: T2}···%{T- A"
suspend="A:systemctl suspend-then-hibernate:}suspend%{A"

if [ "$1" = "open" ]; then
  echo open
  echo "%{+u A:$close:}X%{A -u O20 +u $hibernate -u O20 +u $suspend -u $(./scripts/extra-options.sh)}"
else
  shift
  echo close
  echo "%{+u $open -u O20}$@"
fi

