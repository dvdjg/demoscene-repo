Run from WSL 2:

	sudo apt install ./demoscene-toolchain_2025.09.21+1_amd64.deb
	git lfs install # Fails

	curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
	sudo apt-get update
	sudo apt-get install git-lfs
	git lfs install
	wget https://github.com/cahirwpz/demoscene-toolchain/releases/download/2025-09-21/demoscene-toolchain_2025.09.21+1_amd64.deb
	sudo apt install ./demoscene-toolchain_2025.09.21+1_amd64.deb # Fails
	
	sudo add-apt-repository ppa:deadsnakes/ppa
	sudo apt update
	sudo apt install libpython3.11
	sudo apt install ./demoscene-toolchain_2025.09.21+1_amd64.deb

	sudo mkdir -p /opt/amiga/share/fs-uae/kickstarts
    sudo cp /mnt/e/*.rom  /opt/amiga/share/fs-uae/kickstarts

	cd /mnt/c/Users/David/Documents/Programa/Amiga/demoscene/demoscene-repo/
	

	$ source activate
	Demoscene build environment in /mnt/c/Users/David/Documents/Programa/Amiga/demoscene/demoscene-repo configured for A500!

	$ make
	(.venv) $ code .