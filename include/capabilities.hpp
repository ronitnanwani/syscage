#ifndef CAPABILITIES_HPP
#define CAPABILITIES_HPP

// Drop dangerous Linux capabilities from the bounding and inheritable sets.
// This limits what uid 0 can do inside the container.
int drop_capabilities();

#endif // CAPABILITIES_HPP
