all clean:
	@find . -maxdepth 1 -mindepth 1 -type d -exec sh -c 'cd {} && $(MAKE) $@' \;
