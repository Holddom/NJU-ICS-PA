include nemu/scripts/git.mk

default:
	@echo "Please run 'make' under any subprojects to compile."

submit:
	git gc
	STUID=$(STUID) STUNAME=$(STUNAME) bash -c "$$(curl -s http://jyywiki.cn/static/submit.sh)"
count:
	find nemu/ -name "*.c" -o -name "*.h" | xargs cat | sed '/^\s*$$/d' | grep -v '^$$' | wc -l
.PHONY: default submit
