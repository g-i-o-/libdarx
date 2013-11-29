libdarx
=======

Library for reading and writing darx files.

A darx (short for data archive) file is a set of data tensors, associated with some archive metadata.


Each tensor specifies it's own dimensions and datatype. The purpose of
such a file is to store associated data of any type into one single archive.

Each tensor's data can be compressed individually, allowing the header to still be read.

Also, the format provides built in support for tensors holding mixed and custom data types.

The mixed data type specifies elements as a tuple of elements, each with it's own
recursively defined data type, whereas the custom data type specifies an application-dependent
formatting of the data.

Originally created to support bundling of related matrices in one convenient, portable, file.


