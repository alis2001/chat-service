#!/bin/bash

# ================================================
# CHAT DATABASE INITIALIZATION SCRIPT
# ================================================

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Database connection details
DB_HOST="localhost"
DB_PORT="5434"
DB_NAME="chat_service" 
DB_USER="chat_user"
DB_PASSWORD="admin5026"

echo -e "${BLUE}🚀 Initializing Chat Service Database...${NC}"

# Function to print status
print_status() {
    echo -e "${BLUE}📋 $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Check if Docker containers are running
check_containers() {
    print_status "Checking Docker containers..."
    
    if ! docker ps | grep -q "caffis-chat-db"; then
        print_error "Chat database container is not running!"
        print_status "Please run: docker-compose up -d chat-db"
        exit 1
    fi
    
    print_success "Chat database container is running"
}

# Wait for database to be ready
wait_for_database() {
    print_status "Waiting for database to be ready..."
    
    max_attempts=30
    attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if docker exec caffis-chat-db pg_isready -U $DB_USER -d $DB_NAME >/dev/null 2>&1; then
            print_success "Database is ready!"
            return 0
        fi
        
        echo -e "${YELLOW}⏳ Waiting for database... (${attempt}/${max_attempts})${NC}"
        sleep 2
        ((attempt++))
    done
    
    print_error "Database failed to become ready after ${max_attempts} attempts"
    exit 1
}

# Apply database schema
apply_schema() {
    print_status "Applying database schema..."
    
    if [ ! -f "chat_schema.sql" ]; then
        print_error "chat_schema.sql file not found!"
        print_status "Please ensure chat_schema.sql is in the current directory"
        exit 1
    fi
    
    # Apply schema using docker exec
    if docker exec -i caffis-chat-db psql -U $DB_USER -d $DB_NAME < chat_schema.sql; then
        print_success "Database schema applied successfully!"
    else
        print_error "Failed to apply database schema"
        exit 1
    fi
}

# Verify schema was applied
verify_schema() {
    print_status "Verifying database schema..."
    
    # Check if main tables exist
    tables=("chat_users" "chat_rooms" "messages" "room_participants" "message_read_status")
    
    for table in "${tables[@]}"; do
        if docker exec caffis-chat-db psql -U $DB_USER -d $DB_NAME -t -c "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = '$table');" | grep -q "t"; then
            print_success "Table '$table' exists"
        else
            print_error "Table '$table' is missing!"
            exit 1
        fi
    done
    
    # Check record count
    user_count=$(docker exec caffis-chat-db psql -U $DB_USER -d $DB_NAME -t -c "SELECT COUNT(*) FROM chat_users;" | xargs)
    room_count=$(docker exec caffis-chat-db psql -U $DB_USER -d $DB_NAME -t -c "SELECT COUNT(*) FROM chat_rooms;" | xargs)
    
    print_success "Database verification complete!"
    print_status "📊 Database Stats:"
    echo -e "   • Users: ${user_count}"
    echo -e "   • Rooms: ${room_count}"
}

# Create MinIO buckets
setup_file_storage() {
    print_status "Setting up file storage buckets..."
    
    # Wait for MinIO to be ready
    sleep 5
    
    # Create buckets for different file types
    buckets=("chat-images" "chat-files" "chat-voice")
    
    for bucket in "${buckets[@]}"; do
        # Create bucket using mc (MinIO client) via docker
        if docker run --rm --network chat-network_chat-network \
           -e MC_HOST_minio=http://chatadmin:admin5026@chat-file-storage:9000 \
           minio/mc mb minio/$bucket 2>/dev/null || true; then
            print_success "Bucket '$bucket' created/verified"
        else
            print_warning "Could not create bucket '$bucket' (might already exist)"
        fi
    done
}

# Main execution
main() {
    echo -e "${BLUE}"
    echo "=================================================="
    echo "   CAFFIS CHAT SERVICE DATABASE INITIALIZATION"
    echo "=================================================="
    echo -e "${NC}"
    
    check_containers
    wait_for_database
    apply_schema
    verify_schema
    setup_file_storage
    
    echo -e "${GREEN}"
    echo "=================================================="
    echo "🎉 CHAT DATABASE INITIALIZATION COMPLETE!"
    echo "=================================================="
    echo -e "${NC}"
    
    print_status "📋 Next Steps:"
    echo "   1. Start chat backend: docker-compose up -d chat-backend"
    echo "   2. Check logs: docker logs caffis-chat-backend -f"
    echo "   3. Test API: curl http://localhost:5004/health"
    echo
    print_status "📊 Database Access:"
    echo "   • Database: postgresql://chat_user:admin5026@localhost:5434/chat_service"
    echo "   • File Storage: http://localhost:9001 (admin: chatadmin/admin5026)"
    echo "   • Redis: redis://localhost:6380"
    echo
}

# Run main function
main "$@"