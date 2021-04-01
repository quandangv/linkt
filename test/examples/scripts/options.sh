close="save lemonbar.options.raw.mode=close"
hibernate="A:systemctl hibernate:}Hibernate%{A"
open="A:save lemonbar.options.raw.mode=open: T2}···%{T- A"
suspend="A:systemctl suspend-then-hibernate:}Suspend%{A"

if [ "$1" = "open" ]; then
  echo open
  echo "%{+u A:$close:}X%{A O20 $hibernate O20 $suspend $(./scripts/extra-options.sh) -u}"
else
  shift
  echo close
  echo "%{+u $open -u O20}$@"
fi

