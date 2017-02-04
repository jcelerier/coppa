# Any client -> Server 
```
/da?value&tags
```
etc...

# Server -> Any client

namespace :
```
{
	"full_path" : "/a/b"
	"type": "f"
	"access": 1
	"contents" : [ ]
}
```
=== Server -> Synchronized websocket client ===
```
path_added :
{
	"path_added" : {
		  "full_path" : "/a/b"
	      "type": "f"
		  "access": 1
		  "contents" : [ ] }
}

// Will completely replace
path_changed :
{
	"path_changed" : { full_path : "/a/b" ... }
}

path_removed :
{
	"path_removed" : "/a"
}

// Will only replace the listed attributes
attributes_changed :
{
	"attributes_changed" : {
		"full_path": “/foo/baz”
		"range": [0 150 null]
	}
}


paths_added :
{
	"paths_added" : [
		{ "full_path" : "/a/b"
	      "type": "f"
		  'access": 1
		  "contents" : [ ] },
		{ "full_path" : "/c" ... }
	]
}

// Note:  if multiple patsh changed in the same hierarchy
// the sub-paths have to be after ? -> evaluation in order
path_changed :
{
	"paths_changed" : [
		{ "full_path" : "/a/b" ... },
		{ "full_path" : "/c" ... }
	]
}

paths_removed :
{
	"paths_removed" : ["/a", "/b"]
}

attributes_changed_array :
{
	"attributes_changed_array" : [
		{ "full_path": “/foo/baz”
		  "range": [0 150 null] },
		{ "full_path": “/foo/bar”
		  "tags": ["some" "tags"] },
    ]
}
```

# OSC changes

Streaming osc : rajouter `listen=true&http` si on veut un streaming via websockets ?

Removal root node ?

Differentiation method / non-method : is this the right place ?
Couldn't a method also have childs ?


Shouldn't the root node address be "" to remove side cases ?
