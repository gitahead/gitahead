#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import cgi
import cgitb
import json
import markdown
import os
import sys
from distutils.version import StrictVersion

def fail():
  print("Content-Type: application/json;charset=utf-8\n\n{}")
  sys.exit()

# Enable debugging.
cgitb.enable()

# Validate query arguments.
query = cgi.FieldStorage()
if not "platform" in query:
  fail()

version = query["version"].value if "version" in query else None
platform = query["platform"].value

# Read the changelog file.
text = ""
versions = []
dir = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(dir, "changelog.md")) as file:
  for line in file:
    tokens = line.strip().split(" ")
    if len(tokens) > 1 and tokens[0] == "###":
      ver = tokens[1].lstrip("v")
      if not version:
        version = ver
      elif StrictVersion(ver) <= StrictVersion(version):
        break
      versions.append(ver)
    text += line

if not versions:
  fail()

# Remove trailing horizontal rule.
text = text.rstrip("-\n")

# Format the link.
extension = "sh"
if platform == "mac":
  extension = "dmg"
elif platform.startswith("win"):
  extension = "exe"

link = "https://gitahead.com/downloads/v{}/GitAhead{}-{}.{}".format(
  versions[0], "-{}".format(platform) if platform.startswith("win") else "",
  versions[0], extension)

obj = {
  "link": link,
  "version": versions[0],
  "changelog": markdown.markdown(text)
}

print("Content-Type: application/json;charset=utf-8\n")
print(json.dumps(obj))
