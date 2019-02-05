#! /usr/bin/env bash
set -e
URL=http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/

git status --short | while read; do
  echo "There are uncommitted changes in the repository, aborting.">&2
  exit 1
done

echo "Getting revision list..."
read revisions < <(
  git log --pretty=format:%f \
    | gawk 'BEGIN {IRS="\n";ORS=" "} {print $0} END {ORS="";print "\n"}'
)

echo "Downloading archive list...">&2
read -a archives < <(
  curl --silent "$URL" \
    | gawk 'BEGIN {ORS=" "}  match($0, /<a href="(asl-1.41r[0-9]+|asl-current-[^.]+)\.tar\.bz2">/, cap) { print cap[1] } END {ORS=""; print"\n"}' \
)

for archive in "${archives[@]}"; do
  echo "${revisions}" | grep "${archive}" >/dev/null && continue || true
  
  read -p "No revision ${archive}, download? [Y/n] " -e -i Y
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
  git commit -m "${archive}"
done
