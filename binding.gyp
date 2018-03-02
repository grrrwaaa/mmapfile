{
  "targets": [
    {
      "target_name": "mmapfile",
      "sources": [ "mmapfile.cpp" ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}