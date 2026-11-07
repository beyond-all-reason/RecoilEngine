+++
title = "Resource excess"
author = "sprunk"
+++

### Resource excess
When a player generates more resources in a GameFrame than they can store, the leftovers are called **excess**. (sometimes called overflow)

In some games, the level-that-is-considered-excess can be set to something lower than 100% of storage capacity. In the example below, the "red slider" (excess level) is set to 75% of player-storage.

### Scenario Context

Here's how it works. 

Consider the scenario:
- you have 0/100 storage at frame 1
- you receive a chunk of 70 resource at frame 2 (stored 70/100)
- you receive another chunk of 170 resource at frame 2. 
  - stored 100/100, 140 excess so far
- you consume a chunk of 50 resource at frame 2. 
  - stored 50/100, 140 excess so far
- you attempt to consume a chunk of 51 resource at frame 2. 
  - this fails because you only have 50 stored
- you receive a chunk of 110 resource at frame 2. 
  - 50 goes to storage, 60 goes to excess (200 excess total in frame 2)
- you consume a chunk of 50 resource at frame 3. 
  - ( current stored 50/100 )
- you attempt to consume a chunk of 51 resource at frame 3.  
  - this fails because you only have 50 stored
- you receive a chunk of 300 resource at frame 4 
  - (50 goes to storage, 250 excess which is all for frame 4)
- you have the "red slider" set to 75%

...now, what happens at closest slowupdate? (see below alternatives)

### no excess gadget, sharing modrule = true (current state of affairs)

- you will receive the total excess 450 from frames 2 and 4, and have virtually 550/100
- then, anything above your red slider value of 75 is distributed to allies and you virtually have 75/100
- in case your allies are all above red slider, you may receive some metal back. this can put you at any value above 75
- the stored value is then capped at your max storage of 100.

### excess gadget, sharing modrule = true
- at the end of frame 2 you receive an event for 200 excess (a single event accumulated from that frame despite multiple individual chunks, and despite consumption attempts between them) and can consume it, or not
- at the end of frame 4 you receive an event for 250 excess and can consume it, or not
- you will receive either 0, 200, 250, or 450 excess depending on which of the two excess the gadget did not intercept, and virtually have either 100, 300, 350 or 550
- then anything above your red slider value of 75 is distributed to allies and you virtually have 75/100
- in case your allies are all above red slider, you may receive some metal back. this can put you at any value above 75
the stored value is then capped at your max storage of 100.

# no excess gadget, sharing modrule = false
- you will receive the total excess 450 from frames 2 and 4, and have virtually 550/100
- the stored value is then capped at your max storage of 100.

# excess gadget, sharing modrule = false
- you will receive either 0, 200, 250, or 450 depending on what you didn't intercept, and have 100, 300, 350 or 550 accordingly
-  the stored value is then capped at your max storage of 100.