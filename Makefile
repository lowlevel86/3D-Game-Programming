all:
	make -C 3DCrate
	make -C 3DPlatformer
	make -C 3DSpaceInvaders
	make -C Dbgwin
	make -C Draw

clean:
	make -C 3DCrate clean
	make -C 3DPlatformer clean
	make -C 3DSpaceInvaders clean
	make -C Dbgwin clean
	make -C Draw clean

run:
	make -C 3DCrate run
	make -C 3DPlatformer run
	make -C 3DSpaceInvaders run
	make -C Dbgwin run
	make -C Draw run


