#! /usr/bin/env bash
set -e
URL=http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/
AUTHOR="Alfred Arnold <alfred@ccac.rwth-aachen.de>"

git status --short | while read; do
  echo "There are uncommitted changes in the repository, aborting.">&2
  break
  exit 1
done

echo "Getting revision list..."
read revisions < <(
  git log --pretty=format:%f \
    | gawk 'BEGIN {IRS="\n";ORS=" "} {print $0} END {ORS="";print "\n"}'
)

echo "Downloading archive list...">&2
unset archives times
index=0

while read archive time1 time2; do
  archives[${index}]=${archive}
  times[${index}]="${time1}T${time2}"
  index=$((${index}+1))
done < <(
  curl --silent "$URL" \
    | gawk 'match($0, /<a href="(asl-1.41r[0-9]+|asl-current-[^.]+)\.tar\.bz2">.*([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2})/, cap) { print cap[1] " " cap[2] }' \
)

for index in ${!archives[*]}; do
  archive="${archives[$index]}"
  time="${times[$index]}"
  echo "${revisions}" | grep "${archive}" >/dev/null && continue || true
  
  read -p "No revision ${archive} / ${time}, download? [Y/n] " -e -i Y
  [[ ${REPLY,,} =~ ^y ]] || continue

  case "${archive}" in
  asl-current-142-bld16) # the .tar.bz2 is damaged
    tape="${archive}.tar.gz" ;;
  *)
    tape="${archive}.tar.bz2" ;;
  esac

  echo "Downloading ${tape}"
  curl -O "${URL}/${tape}"

  echo "Testing ${tape}"
  tar -t -f "${tape}" >/dev/null 2>&1 | head -10 >&2

  unset msg
  for f in *; do
    case "$f" in
    COPYING|download.sh|repo|${tape}) ;;
    *)
      if [[ ! $msg ]]; then
        echo "Clearing the repository, beginning with '$f'"
        msg=yes
      fi
      rm -rf -- "./$f";;
    esac
  done

  echo "Decompressing..."
  tar -x -f "${tape}" --strip-components 1
  rm "${tape}"
  
  echo "Committing ${archive}..."
  git add --all
  git commit --allow-empty --author="${AUTHOR}" --date="${time}" --message="${archive}"
done
