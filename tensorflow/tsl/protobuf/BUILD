load(
    "//tensorflow/tsl:tsl.bzl",
    "if_google",
)
load(
    "//tensorflow/tsl/platform:build_config.bzl",
    "tf_proto_library",
)

package(
    default_visibility = [
        "//tensorflow/core:__subpackages__",
        "//tensorflow/tsl:internal",
        "//tensorflow_models:__subpackages__",
    ],
    features = if_google(["-parse_headers"]),
    licenses = ["notice"],
)

tf_proto_library(
    name = "bfc_memory_map_proto",
    srcs = ["bfc_memory_map.proto"],
    make_default_target_header_only = True,
)

tf_proto_library(
    name = "error_codes_proto_impl",
    srcs = ["error_codes.proto"],
    make_default_target_header_only = True,
)

tf_proto_library(
    name = "histogram_proto",
    srcs = ["histogram.proto"],
    make_default_target_header_only = True,
)

tf_proto_library(
    name = "distributed_runtime_payloads_proto",
    srcs = ["distributed_runtime_payloads.proto"],
    make_default_target_header_only = True,
)

tf_proto_library(
    name = "rpc_options_proto",
    srcs = ["rpc_options.proto"],
    make_default_target_header_only = True,
)

tf_proto_library(
    name = "protos_all",
    create_go_proto = False,
    make_default_target_header_only = True,
    protodeps = [
        ":bfc_memory_map_proto",
        ":distributed_runtime_payloads_proto",
        ":error_codes_proto_impl",
        ":histogram_proto",
        ":rpc_options_proto",
    ],
    visibility = ["//visibility:public"],
)
