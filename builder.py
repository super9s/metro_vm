import os
import sys
import json
import glob

def dict_writer(dist: dict, src: dict, overWrite=False, mix=False) -> dict:
    for k in src.keys():
        if not overWrite and k in dist.keys():
            continue

        if mix and k in dist.keys() and type(dist[k]) == type(src[k]) == dict:
            dist[k] = dict_writer(dist[k], src[k], overWrite, mix)
        else:
            dist[k] = src[k]

    return dist


class Builder:
    def __init__(self, name: str, ctx: dict):
        self.name = name
        self.context = ctx
        self.completed = False

    #
    # get_flags():
    #   get flag in self.context
    #  lang: c cpp ld
    # 
    def get_flags(self, lang)
        

    def get_all_sources(self) -> list:
        

class Driver:
    def __init__(self, json_path: str):
        builders = { }

        with open(json_path, mode='r', encoding='utf-8') as fs:
            tmp = json.loads(fs.read())

            for k in tmp.keys():
                builders[k] = Builder(k, tmp[k])


    def build_all(self):
        print("aiueo")


def main(argv):
    d = Driver('build.json')

    d.build_all()


exit(main(sys.argv))
