pipeline {
    agent any
    options {
        // Every archived build carries ~400 MB of packages: keep the logs, drop old artifacts.
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '3'))
    }
    stages {
        stage('Build and run tests') {
            matrix {
                agent {
                    docker {
                        image 'juzzlin/qt6-26.04:latest'
                        args '--privileged -t -v $WORKSPACE:/noteahead'
                    }
                }
                axes {
                    axis {
                        name 'BUILD_TYPE'
                        values 'Debug', 'Release'
                    }
                }
                stages {
                    stage('Build and test') {
                        steps {
                            // Debug is built with line tables only, compressed (-g1 -gz): every test
                            // links the whole program, and at full debug info the test binaries took
                            // ~22 GB per Debug workspace, too much to keep. At a sixth of that they
                            // are kept between runs, so a run relinks only what changed instead of
                            // every test, which was most of this stage's time.
                            sh "mkdir -p build-$BUILD_TYPE && cd build-$BUILD_TYPE && cmake -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE '-DCMAKE_CXX_FLAGS_DEBUG=-g1 -gz' .. && cmake --build . && ctest --output-on-failure --timeout 600 --parallel \$(nproc)"
                            // Starts the UI headless and fails on any QML warning, which no unit test sees
                            sh "cd build-$BUILD_TYPE && QT_QPA_PLATFORM=offscreen ./noteahead --smoke-test"
                        }
                    }
                }
            }
        }
        stage('Debian package / Ubuntu / Qt 6') {
            matrix {
                axes {
                    axis {
                        name 'UBUNTU_VERSION'
                        values '24.04', '26.04'
                    }
                }
                stages {
                    stage('Build and run CPack') {
                        steps {
                            script {
                                def imageName = "juzzlin/qt6-${UBUNTU_VERSION}:latest"
                                docker.image(imageName).inside('--privileged -t -v $WORKSPACE:/noteahead') {
                                    sh "mkdir -p build-deb-ubuntu-${UBUNTU_VERSION}-qt6"
                                    // The tests are built and run in the stage above; built here they would only
                                    // be linked a second time and thrown away.
                                    sh "cd build-deb-ubuntu-${UBUNTU_VERSION}-qt6 && cmake -GNinja -DDISTRO_VERSION=ubuntu-${UBUNTU_VERSION} -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF .. && cmake --build ."
                                    // The build directory is kept between runs: clear the last version's package
                                    sh "cd build-deb-ubuntu-${UBUNTU_VERSION}-qt6 && rm -f *.deb && cpack -G DEB"
                                }
                                // Installed into a stock image, not the build image, which has every
                                // dependency already: a package that forgets one has to fail here.
                                sh "./scripts/smoke-test-deb ubuntu:${UBUNTU_VERSION} build-deb-ubuntu-${UBUNTU_VERSION}-qt6/*.deb"
                            }
                        }
                    }
                }
                post {
                    always {
                        archiveArtifacts artifacts: "build-deb-ubuntu-${UBUNTU_VERSION}-qt6/*.deb", fingerprint: true
                    }
                }
            }
        }        
        stage('Source .tar.gz') {
            agent {
                docker {
                    image 'juzzlin/qt6-24.04:latest'
                    args '--privileged -t -v $WORKSPACE:/noteahead'
                }
            }
            steps {
                sh "./scripts/build-archive"
            }
            post {
                always {
                    archiveArtifacts artifacts: 'noteahead*.tar.gz', fingerprint: true
                }
            }
        }
        stage('AppImage') {
            // Runs on the host agent rather than in a Docker agent: the script
            // starts a container of its own, so that the same command builds the
            // same AppImage here and on a developer's machine.
            steps {
                sh "rm -f Noteahead-*.AppImage"
                sh "./scripts/build-appimage-docker"
                // Each AppImage on its own release, and the portable 24.04 one on Fedora too, with
                // nothing installed but what any desktop has
                sh "./scripts/smoke-test-appimage ubuntu:24.04 Noteahead-*-24.04.AppImage"
                sh "./scripts/smoke-test-appimage ubuntu:26.04 Noteahead-*-26.04.AppImage"
                sh "./scripts/smoke-test-appimage fedora:43 Noteahead-*-24.04.AppImage"
            }
            post {
                always {
                    archiveArtifacts artifacts: 'Noteahead-*.AppImage', fingerprint: true
                }
            }
        }
    }
}
