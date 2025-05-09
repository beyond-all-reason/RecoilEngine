import shutil
import os
import sys

meta_files_path = sys.argv[1]
temp_md_path = sys.argv[2]
output_path = sys.argv[3]

index_text = "" # This is a really dumb way to retain the index from getting deleted

def hugoify(dir: str):
    print("Hugoifying " + dir)
    #os.makedirs(output_path, exist_ok=True)
    for path, folders, files in os.walk(dir):
        for filename in files:
            if (not filename.endswith(".md")):  # skip non-md files
                continue

            if filename.find("index") != -1:  # move index files wholesale
                with open(os.path.join(path, filename)) as src:
                    with open(os.path.join(output_path, "_index.md"), "w") as dest:
                        dest.write(src.read())
                        dest.close()
                    src.close()

                continue

            name_pieces = filename.split(".")[:-1]
            #name = name_pieces[-1]
            directory = os.path.join(output_path, *name_pieces)
            full_path = os.path.join(directory, "_index.md")
            original_path = os.path.join(path, filename)

            os.makedirs(directory, exist_ok=True)
            print("Writing " + full_path)
            with open(full_path, "w") as dest:
                with open(original_path, "r") as src:
                    dest.write(src.read())
                    src.close()
                dest.close()

        for folder in folders:
            hugoify(folder)


def execute():
    index_path = os.path.join(output_path, "_index.md")
    print("temp: {0}, meta: {1}, output: {2}, index: {3}".format(temp_md_path, meta_files_path, output_path, index_path))
    with open(index_path, "r") as idx:
        index_text = idx.read()

    hugoify(temp_md_path)

    with open(index_path, "w") as idx:
        idx.write(index_text)


if __name__ == "__main__":
    execute()
