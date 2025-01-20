## About MeshCore

MeshCore is a portable C++ library which provides classes for adding multi-hop packet routing to embedded projects typically employing packet radios like LoRa.

At present it is mostly aimed at Arduino projects, using the [PlatformIO](https://docs.platformio.org) tools, but could potentially be integrated into other environments.

## Getting Started

You'll need to install PlatformIO, and an IDE in which it runs, like VSCode. Once installed, you should just be able to open this folder as a project, and it will read the platformio.ini file, and bring in all the required dependencies.

There are a number of build environments defined in the platformio.ini file, all targeting the Heltec V3 LoRa boards. For example, **[env:Heltec_v3_chat_alice]** is the target for building/running the 'secure chat' sample app as the user 'Alice'. There is a similar env, configured to run the secure chat as the user 'Bob'.

Try running these two examples first, flashing to two separate Heltec V3's, and use the Serial Monitor to interact with the *very basic* command-line interface.

## Other Example Apps

There is also a pair of examples, **'ping_client'** and **'ping_server'** which has some very basic constructs for setting up a node in a 'server'-like role, and another as client.

There is also a **'simple_repeater'** example, which should function as a basic repeater to ALL of the various samples, like the chat ones. It also defines a few examples of some 'remote admin', like setting the clock. The **'test_admin'** example is an example of an app that remotely monitors and sends commands to the 'simple_repeater' nodes.

## To-Do's

Will hopefully figure out how to make this a registered PlatformIO library, so it can just be added in **lib_deps** in your own project.

## More Resources

You will be able to find additional guides and components at [my site](https://buymeacoffee.com/ripplebiz), or [join Andy Kirby's Discord](https://discord.gg/GBxVx2JMAy) for discussions.

