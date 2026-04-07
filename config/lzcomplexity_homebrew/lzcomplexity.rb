cask "lzcomplexity" do
  version "0.0.0" # CI will update this
  sha256 "0000000000000000000000000000000000000000000000000000000000000000" # CI will update this

  # CI will update this URL to point to the specific release tag
  url "https://github.com/your-username/your-project/releases/download/v0.0.0/lzcomplexity-v0.0.0.pkg"
  
  name "LZComplexity"
  desc "High performance LZ complexity analysis tool"
  homepage "https://github.com/your-username/your-project"

  # Tells Homebrew to install this specific pkg file
  # Since the URL points directly to the .pkg, the filename matches the URL tail
  pkg "lzcomplexity-v0.0.0.pkg"

  # Optional: Uninstall logic using pkgutil (requires knowing your Bundle ID)
  # Uncomment and replace 'com.yourusername.lzcomplexity' with your actual Bundle ID
  uninstall pkgutil: "com.pleros-ai.lzcomplexity"

  # def install
  #   # If CPack creates a root folder named lzcomplexity-1.0
  #   prefix = Dir.glob("*").first 
  #   bin.install "bin/lzcomplexity"
  #   lib.install "#{prefix}/lib/*"
  #   include.install "#{prefix}/include/*"
  # end

  zap trash: [
    "~/.config/lzcomplexity",
  ]
end