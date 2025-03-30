pipeline {
    agent any
    stages {
        stage('Build and run tests') {
            matrix {
                agent {
                    docker {
                        image 'juzzlin/qt6-24.10:latest'
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
                            sh "mkdir -p build-$BUILD_TYPE && cd build-$BUILD_TYPE && cmake -GNinja -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && cmake --build . && ctest"
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
                        values '24.10'
                    }
                }
                stages {
                    stage('Build and run CPack') {
                        steps {
                            script {
                                def imageName = "juzzlin/qt6-${UBUNTU_VERSION}:latest"
                                docker.image(imageName).inside('--privileged -t -v $WORKSPACE:/noteahead') {
                                    sh "mkdir -p build-deb-ubuntu-${UBUNTU_VERSION}-qt6"
                                    sh "cd build-deb-ubuntu-${UBUNTU_VERSION}-qt6 && cmake -GNinja -DDISTRO_VERSION=ubuntu-${UBUNTU_VERSION} -DCMAKE_BUILD_TYPE=Release .. && cmake --build ."
                                    sh "cd build-deb-ubuntu-${UBUNTU_VERSION}-qt6 && cpack -G DEB"
                                }
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
                    image 'juzzlin/qt6-24.10:latest'
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
    }
}
