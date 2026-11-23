// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "PingCoreLibraries",
    platforms: [
        .iOS(.v15),
    ],
    products: [
        .library(
            name: "PingCoreLibraries",
            targets: ["PingCoreLibraries"]
        ),
    ],
    dependencies: [
@PACKAGE_DEPENDENCIES@    ],
    targets: [
        .target(
            name: "PingCoreLibraries",
            dependencies: [
@PRODUCT_DEPENDENCIES@            ]
        ),
    ]
)
